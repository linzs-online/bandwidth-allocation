#include <fstream>
#include <sstream>
#include <algorithm>
#include <list>
#include <cmath>
#include <cassert>
#include <random>
#include <set>
#include "../inc/base.h"
#include "../inc/tic_toc.h"


using namespace std;
string temp,line;


Base::Base(string&& _filePath){
    // 初始化siteNodo（ ）
    siteNodeInit(_filePath + "site_bandwidth.csv");
    // 初始化demand（）
    demandNodeInit(_filePath + "demand.csv");
    // 初始化config（）
    qosConstraintInit(_filePath + "config.ini");
    // 初始化QOS()
    qosInit(_filePath + "qos.csv");
    //log = std::make_shared<SiteLog>(siteNode, siteNodeBandwidth, demand.size());
	// 找到满足qos的site
	findUsableSite();
    // 参数初始化
    paramInit();

    maxFree = floor(demand.size() * 0.05);
    //maxFree = 0;
}

void Base::siteNodeInit(string&& _filePath){
    std::ifstream site_bandwidthfile(_filePath, std::ios::in);
    std::string line,siteNoteName,siteNodeBW;
    getline(site_bandwidthfile, temp);
    int siteNodeBW_int;
    while(getline(site_bandwidthfile,line)){
        istringstream sin(line); //将整行字符串line读入到字符串流istringstream中
        getline(sin, siteNoteName, ',');
        siteNode.push_back(siteNoteName);  // 获得边缘节点名字序列
        sin >> siteNodeBW;
        siteNodeBW_int = atoi(siteNodeBW.c_str());
        siteNodeBandwidth.push_back(siteNodeBW_int);
        pair<string,int> _frame(siteNoteName, siteNodeBW_int);
        siteBandWith.insert(_frame);  // 把边缘节点和它的带宽封装成unordermap
        _optimMap.emplace(std::move(siteNoteName), Optim());
    };
    //cout << "siteNodeBandWith Inited " <<  siteBandWith.at("9") << endl;
}

void Base::demandNodeInit(string&& _filePath){
    std::ifstream demandfile(_filePath, std::ios::in);
    getline(demandfile, line,'\r');
	getline(demandfile, temp,'\n');
    istringstream sin(line);
    getline(sin, temp, ',');
    while(getline(sin, temp,','))
    {
        demandNode.emplace_back(temp); // 获得客户节点的名字序列
    }
    vector<int> demandFrame;
    line.clear();
    size_t frameID = 0;
    while(getline(demandfile,line)){
        istringstream sin(line);
        getline(sin, temp, ','); //吞掉前面的时间戳信息
        while(getline(sin, temp, ',')){
            size_t nodeDemand = atoi(temp.c_str());
            demandFrame.emplace_back(nodeDemand);
        }
        demand.emplace_back(demandFrame); //将所有时间序列的客户节点请求封装成vector
        demandFrame.clear();
    }
    //cout << "demandNode Inited " << demandNode.size() << endl;
}

void Base::qosInit(string&& _filePath){
    std::ifstream qosfile(_filePath, std::ios::in);
    getline(qosfile, temp);
    vector<int> preSite2demand;
    string preSiteNode;
    while(getline(qosfile, line))
    {
        istringstream sin(line);
        getline(sin, preSiteNode, ',');
        while(getline(sin, temp, ',')){
            preSite2demand.emplace_back(atoi(temp.c_str()));
        }
        pair<string,vector<int>> pre_site2demand(preSiteNode,preSite2demand);
        site2demand.insert(pre_site2demand);
        preSite2demand.clear();
    }
    vector<int> preDemand2site;
    auto index = 0;
    for(auto dd: demandNode){
        for(auto ss : siteNode){
            preDemand2site.emplace_back(site2demand.at(ss)[index]);
        }
        index++;
        pair<string,vector<int>> pre_demand2site(dd,preDemand2site);
        demand2site.insert(pre_demand2site);
        preDemand2site.clear();
    }
    
    for(auto& sName : siteNode){
        vector<string> _temp; //用于存储当前边缘节点可以被请求的客户节点
        for(auto index = 0; index < demandNode.size(); index++){
            if(site2demand.at(sName)[index] < qos_constraint){
                _temp.emplace_back(demandNode[index]);
            }
        }
        if(_temp.empty()) continue;
        int _size = _temp.size();
        pair<size_t,string> _t(_size,sName);
        pair<string,vector<string>> _T(sName,_temp);
        siteConnetDemand.insert(_T);
        siteConnetDemandSize.insert(_t);
    }

    //cout << "preSite2demand Inited " << site2demand.at("A")[2] << endl;
}


void Base::qosConstraintInit(string&& _filePath){
    std::ifstream qos_constraintfile(_filePath, std::ios::in);
    while(getline(qos_constraintfile,temp,'='));
    qos_constraint = atoi(temp.c_str());
    //cout << "qos_constraint = " <<  qos_constraint << endl;
}
/**
 * @brief 把每帧的结果放在一个vector里面，里面的存放形式是：客户节点-分配方案
 */
void Base::getPreFrameResult(vector<unordered_map<string,vector<pair<string,int>>>>& _preFrameResult,
                             vector<vector<pair<string, int>>>& result){
    unordered_map<string,vector<pair<string,  int>>> _preFramePreDemandPlan;
    //用总帧数量指定存放结果的vector大小
    int demandNodeSize = demandNode.size();
    int index = 0;  //客户节点名称序号
    for(auto _result: result){
        //客户节点 - 分配方案
        pair<string,vector<pair<string,  int>>> _temp(demandNode[index++],_result);
        _preFramePreDemandPlan.insert(_temp);
        if(index == demandNodeSize){ //每帧检查
            _preFrameResult.emplace_back(_preFramePreDemandPlan);//已经取完一帧的数据了
            index = 0;  //序号清零
            _preFramePreDemandPlan.clear(); //把每帧的分配结果缓冲清零
        }
    }
}


/**
 * @brief 获取得分
 * 
 * @return unsigned int 
 */
uint32_t Base::getScore(){
    unordered_map<string,vector<unsigned int>> siteNodeConsume;
    for(auto& siteN : siteNode){
        vector<unsigned int> temp0;
        pair<string,vector<unsigned int>> temp1(siteN, temp0);
        siteNodeConsume.insert(temp1);
    }
    //vector<vector<pair<string, int>>> result;
    for(auto& site : _optimMap){
        string siteName = site.first;
        for(size_t frameID = 0; frameID < site.second.value.size(); frameID ++){
            siteNodeConsume.at(siteName).push_back(computeSiteSumTime(siteName, frameID));
        }
    }
    unsigned int _95nth = ceil(demand.size() * 0.95); //求百分之九十五位
    unordered_map<string,int> preSiteNode95thConsume;
    for(auto _siteNodeName : siteNode){
        nth_element(siteNodeConsume.at(_siteNodeName).begin(),
                    siteNodeConsume.at(_siteNodeName).begin()+_95nth-1,
                    siteNodeConsume.at(_siteNodeName).end());
        pair<string,int> _temp(_siteNodeName,siteNodeConsume.at(_siteNodeName)[_95nth-1]);
        preSiteNode95thConsume.insert(_temp);
    }
    unsigned int totalConsume = 0;
    for(unordered_map<string,int>::iterator it = preSiteNode95thConsume.begin(); it != preSiteNode95thConsume.end(); it++){
        totalConsume += it->second;
    }
    return totalConsume;
}


void Base::paramInit() {
    _layers.reserve(demandNode.size() * demand.size());
    for (size_t i = 0; i < demandNode.size() * demand.size(); ++i) {
        Paramerter::Ptr l = std::make_shared<Paramerter>();
        int node = i % demandNode.size();
        int number = usableSite[demandNode[node]].size();
        l->value.assign(number, 1);
        // l->init(number);
        l->normal();
        l->softmax();
        _layers.emplace_back(std::move(l));
    }
    for (auto& s: siteNode) {
        _optimMap.emplace(s, Optim());
    }
}


void Base::save(string&& _fileName) {
    ofstream file(_fileName, std::ios_base::out);
    int customerNum = demandNode.size();
    for(size_t i = 0; i < result.size(); ++i) {
        int curCustomerID = i % customerNum;
        // 客户ID
        file << demandNode[curCustomerID] << ":";
        if (result[i].empty()) {
            if (i < result.size() - 1) {
                file << "\n";
            }
            continue;
        } 
        // 分配方案, 0不输出
		bool flag = false;
        for (size_t j = 0; j < result[i].size(); ++j) {
			if (result[i][j].second != 0) {
				if (flag) {
					file << ",";
				} else {
					flag = true;
				}
				file << "<" << result[i][j].first << ","
					 << result[i][j].second << ">";
			}
        }
		file << "\n";
    }
    file.close();
}

void Base::findUsableSite() {
	for (size_t i = 0; i < demandNode.size(); ++i) {
		auto site = demand2site.at(demandNode[i]);
		vector<int> usableCustom2Site;
		for(size_t j = 0; j < site.size(); ++j){
			if(site[j] < qos_constraint){
				usableCustom2Site.push_back(j);
			}
		}
		pair<string, vector<int>> tempSite(demandNode[i], usableCustom2Site);
		usableSite.insert(tempSite);
	}

}

// 获得上帝视角
//map<ID,map<客户名字，需求>>
//map<ID,map<边缘节点名字，剩余带宽>>
inline void getFrameMap(vector<string>& demandNode,
                        unordered_map<string,int>& siteBW,
                        vector<vector<int>>& demand,
                        unordered_map<uint32_t, unordered_map<string, uint64_t>>& frameMap,
                        unordered_map<uint32_t, unordered_map<string,int>>& sitePerFrameBW){
    for(size_t id = 0; id < demand.size(); id++){
        unordered_map<string, uint64_t> Temp;
        for(size_t index = 0; index < demand[id].size(); index++){
            pair<string, uint64_t> temp(demandNode[index],demand[id][index]);
            Temp.insert(temp);
        }
        pair<uint32_t, unordered_map<string, uint64_t>> _T(id,Temp);
        frameMap.insert(_T);
        pair<uint32_t, unordered_map<string,int>> siteTemp(id, siteBW);
        sitePerFrameBW.insert(siteTemp);
    }
}

inline void initalazeSortMap(vector<string>& site, 
                        unordered_map<string, multimap<uint64_t, uint32_t>>& sortMap){
    for(auto& sN : site){
        pair<uint64_t,uint32_t> _temp;
        multimap<uint64_t,uint32_t> T;
        T.insert(_temp);
        pair<string, multimap<uint64_t,uint32_t>> _Temp(sN, T);
        sortMap.insert(_Temp);
    }
}

//map<uint32_t,unordered_map<string,unordered_map<string, pair<string,uint64_t>>>> bestResult;
void Base::initalzeResult(uint16_t&& size){
    for(auto i = 0; i < size; i++){
        unordered_map<string,unordered_map<string, pair<string,int>>> temp4;
        for(auto& demandName : demandNode){
            unordered_map<string, pair<string,int>> temp0;
            for(auto& index : usableSite.at(demandName)){
                string siteName = siteNode[index];
                pair<string, int> temp1(siteName, 0);
                pair<string, pair<string,int>> temp2(siteName, temp1);
                temp0.insert(temp2);
            }
            pair<string,unordered_map<string, pair<string,int>>> temp3(demandName,temp0);
            temp4.insert(temp3);
        }
        pair<uint32_t,unordered_map<string,unordered_map<string, pair<string,int>>>> temp5(i, temp4);
        bestResult.insert(temp5);
    }
}


inline void regulateSortMap(unordered_map<uint32_t, unordered_map<string, uint64_t>>& frameMap,
                            string& siteNode,
                            vector<string>& _siteConnetDemand,
                            multimap<uint64_t, uint32_t>& sortResult){
    for(auto& _frameMap : frameMap){
        uint64_t demandSum = 0;
        for(auto& _dName : _siteConnetDemand){
            demandSum += _frameMap.second.at(_dName);
        }
        pair<uint64_t, uint32_t> temp(demandSum, _frameMap.first);
        sortResult.insert(temp);
    }
}

template<typename T>
int getIdx(vector<T>& vec, T value) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == value) {
            return i;
        }
    }
    return -1;
}


void Base::solveFree(){
    getFrameMap(demandNode, siteBandWith, demand, frameMap, sitePerFrameBW);
    initalazeSortMap(siteNode, sortMap);
    initalzeResult(demand.size()); //初始化结果存储结构
    list<string> freeSite; //最后一个元素中可以连上的客户最少
    for(auto it = siteConnetDemandSize.rbegin(); it != siteConnetDemandSize.rend(); it++){
        freeSite.push_back(it->second); // 边缘节点名字,倒序
    }
    for(auto& siteName : freeSite){ // 遍历可白嫖边缘节点
        multimap<uint64_t, uint32_t> demandSum_id;
        vector<uint32_t> siteFixedTime;
        regulateSortMap(frameMap, siteName, siteConnetDemand.at(siteName), demandSum_id);
        //每个边缘节点有五次免费机会
        multimap<uint64_t, uint32_t>::reverse_iterator it = demandSum_id.rbegin();
        for(auto i = 0; i < maxFree; i++){
            uint32_t frameID = it->second;
            uint64_t demandSum = it->first;
            siteFixedTime.push_back(frameID);
            // 如果总需求小于这个边缘节点的带宽,直接满足所有请求
            if(demandSum <= sitePerFrameBW.at(frameID).at(siteName)){
                for(auto demandName : siteConnetDemand.at(siteName)){
                    bestResult.at(frameID).at(demandName).at(siteName).second = frameMap.at(frameID).at(demandName);
                    sitePerFrameBW.at(frameID).at(siteName) -= frameMap.at(frameID).at(demandName);
                    frameMap.at(frameID).at(demandName) = 0;
                }
            }
            else{
                for(auto demandName : siteConnetDemand.at(siteName)){
                    if(sitePerFrameBW.at(frameID).at(siteName) == 0) break;
                    if(frameMap.at(frameID).at(demandName) == 0) continue;
                    //如果这个边缘节点的带宽小于这个客户的请求
                    if(sitePerFrameBW.at(frameID).at(siteName) < frameMap.at(frameID).at(demandName)){
                        bestResult.at(frameID).at(demandName).at(siteName).second = sitePerFrameBW.at(frameID).at(siteName);
                        frameMap.at(frameID).at(demandName) -= sitePerFrameBW.at(frameID).at(siteName);
                        sitePerFrameBW.at(frameID).at(siteName) = 0;
                    }
                    else{// 如果这个边缘节点的带宽大于这个客户的请求就满足它
                        bestResult.at(frameID).at(demandName).at(siteName).second = frameMap.at(frameID).at(demandName);
                        sitePerFrameBW.at(frameID).at(siteName) -= frameMap.at(frameID).at(demandName);
                        frameMap.at(frameID).at(demandName) = 0;
                    }
                }
            }
            ++it;
        } 
        pair<string, vector<uint32_t>> Temp(siteName, siteFixedTime);
        fixedTime.insert(Temp);
    }// 至此，所有白嫖完成
    auto theMapSum = [&](unordered_map<string,uint64_t> demandMap)->uint64_t{
        uint64_t sum = 0;
        for(auto it = demandMap.begin(); it != demandMap.end(); it++){
            sum += it->second;
        }
        return sum;
    };
    multimap<uint64_t, uint32_t> frameDemandSum;
    for(auto& frame : frameMap){
        uint64_t sum = theMapSum(frame.second);
        pair<uint64_t, uint32_t> temp(sum, frame.first);
        frameDemandSum.insert(temp);
    }
    for(auto frameIt = frameDemandSum.rbegin(); frameIt != frameDemandSum.rend(); frameIt++){
        uint32_t frameID = frameIt->second;
        //uint64_t theSum = theMapSum(frameIt->second);
        unordered_map<string, int> siteBandWithCopy = sitePerFrameBW.at(frameID);
        //if(theSum > 0){
            for(auto& dmandMap : frameMap.at(frameID)){
                if(dmandMap.second == 0) continue;
                vector<int> canUseSite = usableSite.at(dmandMap.first);
                Paramerter::Ptr w = std::make_shared<Paramerter>();
                vector<pair<string, int>> resResult;
                w->value.assign(canUseSite.size(), 1);
                for (auto& s: siteBandWithCopy) {
                    if (s.second == 0) {
                        int id = getIdx(siteNode, s.first);
                        int dis = getIdx(canUseSite, id);
                        w->value[dis] = 0;
                    }
                }
                w->softmax();
                _weightMethod(dmandMap.second, siteBandWithCopy, canUseSite, resResult, w);
                frameMap.at(frameID).at(dmandMap.first) = 0;
                for(auto& _r : resResult){
                    bestResult.at(frameID).at(dmandMap.first).at(_r.first).second += _r.second;
                }
            }
        //}
    } //全部处理完毕，准备处理结果从 bestResult - > result
    //goodResult = bestResult;
    unordered_map<string, Optim::DataType> siteFrameLog; // 记录器，记录单时刻边缘节点情况
    for (auto& s: siteNode) {
        siteFrameLog.emplace(s, Optim::DataType());
    }
    for (size_t i = 0; i < demand.size(); ++i) {
        auto& frame = bestResult[i];
        for (auto& name: demandNode) {
            for (auto& v: frame.at(name)){
                siteFrameLog.at(v.first).emplace_back(&(v.second.second));
            }
        }
        for (auto& s: siteFrameLog) {
            _optimMap.at(s.first).value.emplace_back(std::move(s.second));
        }
    }
    // bestScore = getScore();
}


void Base::solveFree2(){
    getFrameMap(demandNode, siteBandWith, demand, frameMap, sitePerFrameBW);
    initalazeSortMap(siteNode, sortMap);
    initalzeResult(demand.size());//初始化结果存储结构
    list<string> freeSite; //最后一个元素中可以连上的客户最少
    for(auto it = siteConnetDemandSize.rbegin(); it != siteConnetDemandSize.rend(); it++){
        freeSite.push_back(it->second); // 边缘节点名字,倒序
    }
    // vector<uint32_t> siteFixedTime(maxFree);
    // for(auto& siteName : siteNode){
    //     pair<string, vector<uint32_t>> temp(siteName, siteFixedTime);
    //     fixedTime.insert(temp);
    // }
    for(size_t i = 0; i < maxFree; i++){
        for(auto& siteName : freeSite){ // 遍历可白嫖边缘节点
            multimap<uint64_t, uint32_t> demandSum_id;
            regulateSortMap(frameMap, siteName, siteConnetDemand.at(siteName), demandSum_id);
            multimap<uint64_t, uint32_t>::reverse_iterator it = demandSum_id.rbegin();
            uint32_t frameID = it->second;
            uint64_t demandSum = it->first;
            // fixedTime.at(siteName)[i] = frameID;
            // 如果总需求小于这个边缘节点的带宽,直接满足所有请求
            if(demandSum <= sitePerFrameBW.at(frameID).at(siteName)){
                for(auto& demandName : siteConnetDemand.at(siteName)){
                    bestResult.at(frameID).at(demandName).at(siteName).second = frameMap.at(frameID).at(demandName);
                    sitePerFrameBW.at(frameID).at(siteName) -= frameMap.at(frameID).at(demandName);
                    frameMap.at(frameID).at(demandName) = 0;
                }
            }
            else{
                for(auto& demandName : siteConnetDemand.at(siteName)){
                    if(sitePerFrameBW.at(frameID).at(siteName) == 0) break;
                    if(frameMap.at(frameID).at(demandName) == 0) continue;
                    //如果这个边缘节点的带宽小于这个客户的请求
                    if(sitePerFrameBW.at(frameID).at(siteName) < frameMap.at(frameID).at(demandName)){
                        bestResult.at(frameID).at(demandName).at(siteName).second = sitePerFrameBW.at(frameID).at(siteName);
                        frameMap.at(frameID).at(demandName) -= sitePerFrameBW.at(frameID).at(siteName);
                        sitePerFrameBW.at(frameID).at(siteName) = 0;
                    }
                    else{// 如果这个边缘节点的带宽大于这个客户的请求就满足它
                        bestResult.at(frameID).at(demandName).at(siteName).second = frameMap.at(frameID).at(demandName);
                        sitePerFrameBW.at(frameID).at(siteName) -= frameMap.at(frameID).at(demandName);
                        frameMap.at(frameID).at(demandName) = 0;
                    }
                }
            }
        // 至此，所有白嫖完成
        }
    }
    // auto theMapSum = [&](unordered_map<string,uint64_t> demandMap)->uint64_t{
    //     uint64_t sum = 0;
    //     for(auto it = demandMap.begin(); it != demandMap.end(); it++){
    //         sum += it->second;
    //     }
    //     return sum;
    // };
    // multimap<uint64_t, uint32_t> frameDemandSum;
    // for(auto& frame : frameMap){
    //     uint64_t sum = theMapSum(frame.second);
    //     pair<uint64_t, uint32_t> temp(sum, frame.first);
    //     frameDemandSum.insert(temp);
    // }
    for(auto frameIt = frameMap.begin(); frameIt != frameMap.end(); frameIt++){
        uint32_t frameID = frameIt->first;
        //uint64_t theSum = theMapSum(frameIt->second);
        unordered_map<string, int> siteBandWithCopy = sitePerFrameBW.at(frameID);
        //if(theSum > 0){
            for(auto& dmandMap : frameMap.at(frameID)){
                if(dmandMap.second == 0) continue;
                vector<int> canUseSite = usableSite.at(dmandMap.first);
                Paramerter::Ptr w = std::make_shared<Paramerter>();
                vector<pair<string, int>> resResult;
                w->value.assign(canUseSite.size(), 1);
                for (auto& s: siteBandWithCopy) {
                    if (s.second == 0) {
                        int id = getIdx(siteNode, s.first);
                        int dis = getIdx(canUseSite, id);
                        w->value[dis] = 0;
                    }
                }
                w->softmax();
                _weightMethod(dmandMap.second, siteBandWithCopy, canUseSite, resResult, w);
                frameMap.at(frameID).at(dmandMap.first) = 0;
                for(auto& _r : resResult){
                    bestResult.at(frameID).at(dmandMap.first).at(_r.first).second += _r.second;
                }
            }
        //}
    } //全部处理完毕，准备处理结果从 bestResult - > result
}


void Base::save2(string&& _fileName) {
    ofstream file(_fileName, std::ios_base::out);
    int customerNum = demandNode.size();
    for (size_t i = 0; i < demand.size(); ++i) {
        auto& frame = bestResult[i];
        for (auto& de: frame) {
            file << de.first << ":";
            bool flag = false;
            for (auto& s: de.second) {
                // 分配方案, 0不输出
                if (s.second.second != 0) {
                    if (flag) {
                        file << ",";
                    } else {
                        flag = true;
                    }
                    file << "<" << s.second.first << ","
                        << s.second.second << ">";
                }
            }
            file << "\n";
        }
    }
    file.close();
}


/**
 * @brief 查询边缘节点已分配带宽
*/
static inline int _totalBand(const string& key, const
            unordered_map<string, int>& record) {
    if (record.count(key)) {
        return record.at(key);
    }
    return 0;
}


/**
 * @brief 权重策略进行分配
 * @param demanNow 某个客户需求量
 */
void Base::_weightMethod(int demandNow,
            vector<int>& siteNodeBand,
            const vector<int>& demandUsableSite,
            vector<pair<string, int>>& result,
            Paramerter::Ptr weight,
            unordered_map<string, Optim::DataType>& siteFrameLog) {
    unordered_map<string, int> record;

    _weightDistribution(demandNow, demandNow, siteNodeBand,
                demandUsableSite, record, weight);
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        int site = demandUsableSite[i];
        int band = _totalBand(siteNode[site], record);
        // 权重记录
        // Optim::BaseType log(band, weight->value[i]);
        // siteFrameLog.at(siteNode[site]).emplace_back(std::move(log));
        // 分配方案保存
        if (band != 0) {
            pair<string, int> tmp(siteNode[site], band);
            result.emplace_back(std::move(tmp));
        }
    }
}

void Base::_weightMethod(int demandNow,
            unordered_map<string, int>& siteBandWithCopy,
            const vector<int>& demandUsableSite,
            vector<pair<string, int>>& result,
            Paramerter::Ptr weight) {
    unordered_map<string, int> record;
    vector<int> siteNodeBand;
    for (auto& s: siteNode) {
        siteNodeBand.emplace_back(siteBandWithCopy.at(s));
    }
    _weightDistribution(demandNow, demandNow, siteNodeBand,
                demandUsableSite, record, weight);
    int total = 0;
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        int site = demandUsableSite[i];
        int band = _totalBand(siteNode[site], record);
        pair<string, int> tmp(siteNode[site], band);
        result.emplace_back(std::move(tmp));
    }
    for (size_t i = 0; i < siteNode.size(); ++i) {
        siteBandWithCopy[siteNode[i]] = siteNodeBand[i];
    }
}


/**
 * @brief 更新record, weight
*/
static inline bool _updateRecord(
            Paramerter::Ptr weight,
            const int id,
            unordered_map<string, int>& record,
            const string& key,
            const int totalDemand,
            const int band) {
    int planValue = weight->value[id] * totalDemand;
    int reactValue = _totalBand(key, record) + band;
    float oldWeight = weight->value[id];
    record[key] = reactValue;
    // 仅在边缘节点无法满足计划时分配
    if (reactValue < planValue) {
        weight->value[id] = float(reactValue) / totalDemand;
        if (weight->value[id] - oldWeight != 0.0f) {
            weight->softmax(id + 1, oldWeight - weight->value[id]);
        }
        return true;
    }
    return false;
}

/**
 * @brief 获取需求量余数
*/
static inline uint32_t _getRes(
        const Paramerter::Ptr weight,
        const int totalDemand) {
    uint32_t sum = 0;
    for (size_t i = 0; i < weight->value.size(); ++i) {
        sum += totalDemand * weight->value[i];
    }
    return totalDemand - sum;
}


/**
 * @brief 余数分配
*/
static inline void allcoRes(int& demandNow, int res,
            vector<int>& siteNodeBand,
            const vector<int>& demandUsableSite,
            const vector<string>& siteNode,
            unordered_map<string, int>& record) {
    int resInital = res;
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        int site = demandUsableSite[i];
        if (siteNodeBand[site] == 0) {
            continue;
        }
        if (siteNodeBand[site] >= res) {
            siteNodeBand[site] -= res;
            record[siteNode[site]] = _totalBand(siteNode[site], record) + res;
            res = 0;
        } else {
            res -= siteNodeBand[site];
            record[siteNode[site]] = _totalBand(siteNode[site], record) + siteNodeBand[site];
            siteNodeBand[site] = 0;
        }
        if (res == 0) {
            break;
        }
    }
    demandNow -= resInital;
}


void Base::_weightDistribution(int demandNow,
                const int totalDemand,
                vector<int>& siteNodeBand,
                const vector<int>& demandUsableSite,
                unordered_map<string, int>& record,
                Paramerter::Ptr weight) const {
    // 权重策略
    const int demandInitial = demandNow;
    int res = _getRes(weight, demandInitial);
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        if (demandNow == 0) {
            return; //递归终止
        }
        int site = demandUsableSite[i];
        assert(!isnan(weight->value[i]));
        int allocDemand = demandInitial * weight->value[i];
        if (weight->value[i] == 0 || allocDemand == 0) {
            continue;
        }
        // 获得边缘节点分配量
        int band = 0;
        auto getBand = [&](int* demand) {
            int tmp = band;
            if (*demand == 0) {
                return;
            }
            if (siteNodeBand[site] >= *demand) {
                band += *demand;
                siteNodeBand[site] -= *demand;
            } else {
                band += siteNodeBand[site];
                siteNodeBand[site] = 0;
            }
        };
        getBand(&allocDemand);
        // 更新
        demandNow -= band;
        bool update = _updateRecord(weight, i, record, siteNode[site],
                        totalDemand, band);
        if (update) {
            // 余数调整
            res = _getRes(weight, demandInitial);
        }
    }
    // 余数分配
    allcoRes(demandNow, res, siteNodeBand, demandUsableSite, siteNode, record);
    // 递归分配余量
    _weightDistribution(demandNow, totalDemand,siteNodeBand,
            demandUsableSite, record, weight);
}


// 生成需要修改权重的层数
vector<size_t> randomLayer(size_t min_val,size_t max_val, size_t num) {
	vector<size_t> randomVec;

	for (int i = 0; i < num; ++i) {
		size_t num_val = rand()%(max_val-min_val+1) + min_val;
		randomVec.push_back(num_val);
	}

	return randomVec;
}



uint64_t randomNumInt(uint64_t min, uint64_t max) {
	auto time = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 gen(time);
	uniform_int_distribution<uint64_t> dist(min, max);

	return dist(gen);
}

set<uint64_t> genRandomTimeSet(uint64_t min, uint64_t max, int num) {
	set<uint64_t> randomTimeVec;
	for (int i = 0; i < num; ++i) {
		randomTimeVec.insert(randomNumInt(min, max));
	}

	return randomTimeVec;
}

double randomNumDouble(double min, double max) {
	auto time = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 gen(time);
	uniform_real_distribution<double> dist(min, max);

	return dist(gen);
}

uint64_t findSiteIndex(vector<string> siteNode, const string& site) {
	auto it = find(siteNode.begin(), siteNode.end(), site);
	if(it == siteNode.end()) {
		return -1;
	}

	return distance(siteNode.begin(), it);
}

int findDemandIndex(vector<string> demandNode, const string &demandName) {
	auto it = find(demandNode.begin(), demandNode.end(), demandName);
	if(it == demandNode.end()) {
		return -1;
	}

	return distance(demandNode.begin(), it);
}

/**
 *
 * @param modifiedSite 已经被使用边缘节点的集合
 * @param demandConnectSite
 * @param FrameID
 * @return
 */
vector<int> Base::findUsableSiteIndex(vector<int> modifiedSite, const vector<int>& demandConnectSite, uint64_t FrameID, int selfID) {
	vector<int> usableSiteIndex;
	for(auto siteID : demandConnectSite) { // 遍历可用边缘节点
		if(find(modifiedSite.begin(), modifiedSite.end(), siteID) == modifiedSite.end() and (siteID != selfID)) { // 边缘节点没有被修改，且不是这个边缘节点自己
			string siteName = siteNode[siteID];
			vector<uint32_t> thisSiteFixedTimeVec = fixedTime.at(siteName);
			if(find(thisSiteFixedTimeVec.begin(), thisSiteFixedTimeVec.end(), FrameID) == thisSiteFixedTimeVec.end()) { // 该时刻边缘节点不是最大分配
				usableSiteIndex.emplace_back(siteID);
			}
		}
	}

	return usableSiteIndex;
}


int Base::computeSiteSumTime(const string& siteName, int FrameID) {
	auto optim = _optimMap.at(siteName).value;
//	auto sum = accumulate(optim[FrameID].begin(), optim[FrameID].end(), 0,
//						  [] (int a, int * b) {return (a + *b);});
	int sum = 0;
	for(auto it = optim[FrameID].begin(); it != optim[FrameID].end(); ++it) {
		sum += **it;
	}

	return sum;
}

/**
 * @brief 挑选与平均值偏差最大的时刻
 * @param timeSumMap 边缘节点不同时刻的带宽量
 * @param avr
 * @param num
 * @return
 */
vector<int> findModifyTime(unordered_map<uint64_t , int> timeSumMap, int avr, int num) {
	multimap<int, uint64_t, greater<int>> biasMap;
	vector<int> biasID;
	for(auto timeSum : timeSumMap) {
		uint64_t time = timeSum.first;
		int sum = timeSum.second;
		int bias = abs(sum - avr);
		pair<int, uint64_t> timeBias(bias, time);
		biasMap.insert(timeBias);
	}
//	sort(biasMap.begin(), biasMap.end(), [] (pair<uint64_t, int> a, pair<uint64_t, int> b) {return a.second > b.second;});
	auto it = biasMap.begin();
	for (int i = 0; i < num; ++i) {
		biasID.emplace_back(it->second);
		++it;
	}

	return biasID;
}

uint64_t findModifyDemand(vector<int *> _optim, bool flag) {
	uint64_t ID = 0;
	auto maxIter = max_element(_optim.begin(), _optim.end(), [] (int *a, int *b) {return *a < *b;});
	auto minIter = min_element(_optim.begin(), _optim.end(), [] (int *a, int *b) {return *a < *b;});
	if(flag == true) { // 挑最大
		ID = distance(_optim.begin(), maxIter);
	}
	else{
		ID = distance(_optim.begin(), minIter);
	}

	return ID;
}

void Base::updateBandwidth(int L) {
	int maxL = 2000; // 最大迭代次数
//	double alpha = exp(L / maxL); // 随迭代次数下降的系数
	double alpha = randomNumDouble(1e-16, 1.0);
	for (auto it = siteConnetDemandSize.rbegin(); it != siteConnetDemandSize.rend(); ++it) { // 遍历边缘节点，siteConnetDemandSize按照从小到大排序，后面的是连接客户节点多的边缘节点
		vector<int> modifiedSite; // 记录修改过的边缘节点的序号
		string siteName = it->second;
		uint64_t siteIndex = findSiteIndex(siteNode, siteName);
		uint64_t siteConnectSize = it->first;
		auto& optim = _optimMap.at(siteName).value; // 当前边缘节点的vec，保存不同时刻的带宽量和权重
		vector<uint32_t> siteFixedTime = fixedTime.at(siteName);
		int siteSum = 0;
		int cnt = 0;
		unordered_map<uint64_t , int> siteSumTimeMap; // 存储边缘节点在各时刻带宽总量的map,索引为时刻，value为带宽量
		// 计算当前边缘节点的平均值
		for (uint64_t i = 0; i < optim.size(); ++i) { // 遍历时刻
			// 若该时刻不能修改，跳过
			if(std::find(siteFixedTime.begin(), siteFixedTime.end(), i) != siteFixedTime.end()) {
				continue;
			}
			// 计算一个时刻的和
			auto temp = accumulate(optim[i].begin(), optim[i].end(), 0,
								   [] (int a, int * b) {return (a + *b);});
			siteSum += temp;
			++cnt;
			pair<uint64_t , int> timeSum(i, temp);
			siteSumTimeMap.insert(timeSum);
		}
		int thisSiteAvr = floor(siteSum / cnt);
        for (auto& o: _optimMap) {
            for (auto&s: o.second.value) {
                for (auto& a: s) {
                    int x = *a;
                    assert(x >= 0);
                }
            }
        }

		// 对边缘节点某些时刻的带宽量进行修改，只对随机选择到的时刻且不是固定时刻进行修改。修改应该是固定时刻，固定客户节点的进行修改，才能保证客户节点的需求量不变
//		auto modifiedTimeSet = genRandomTimeSet(0, optim.size() - 1, optim.size() * 0.6);
		const auto& modifyTimeVec = findModifyTime(siteSumTimeMap, thisSiteAvr, floor(optim.size() * 0.5));
//		for (uint64_t i = 0; i < optim.size(); ++i) {
		for(const auto& i : modifyTimeVec){
			// 最大分配和没随机到的时刻都跳过
			if(std::find(siteFixedTime.begin(), siteFixedTime.end(), i) != siteFixedTime.end()) {
				continue;
			}
			uint64_t modifyDemand = 0;
			if(siteSumTimeMap.at(i) > thisSiteAvr) { // 时刻带宽量比平均值大，要减，挑一个分配带宽量最多的客户节点
				modifyDemand = findModifyDemand(optim[i], true);
			} else{
				modifyDemand = findModifyDemand(optim[i], false);
			}
//			uint64_t modifyDemand = randomNumInt(0, siteConnectSize - 1); // 在该时刻里随机一个要修改的客户节点的带宽量
			string thisDemandName = siteConnetDemand.at(siteName)[modifyDemand];
			vector<int> thisDemandConnectSite = usableSite.at(thisDemandName);
			vector<int> thisDemandUsableSiteVec = findUsableSiteIndex(modifiedSite, thisDemandConnectSite, i, siteIndex);
			if(thisDemandUsableSiteVec.empty()) { // 这个客户节点没有能用的边缘节点了，放过他吧
				continue;
			}
			int val = abs(siteSumTimeMap.at(i) - thisSiteAvr) * alpha;
			int perSiteModifyVal = floor(val / thisDemandUsableSiteVec.size());
			if(perSiteModifyVal < 1) { // 每个边缘节点要修改的连1都不到，算了
				continue;
			}

			if(siteSumTimeMap.at(i) <= thisSiteAvr) { // 比平均值小，加
				if((siteNodeBandwidth[siteIndex] - siteSumTimeMap.at(i)) >= val) { // 边缘节点有足够的剩余带宽进行加
					*optim[i][modifyDemand] += perSiteModifyVal * thisDemandUsableSiteVec.size(); // 这个边缘节点加了，其他边缘节点就要减
					if(*optim[i][modifyDemand] < 0) {
						cout << "0" << endl;
					}
					for (auto siteID: thisDemandUsableSiteVec) { // 遍历该客户节点可以修改的边缘节点
						string thisSiteName = siteNode[siteID];
						int thisDemandID = findDemandIndex(siteConnetDemand.at(thisSiteName), thisDemandName);
						if (*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] > perSiteModifyVal) { // 够减
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] -= perSiteModifyVal;
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << "1" << endl;
							}
						} else { // 不够减， 把能减的减了， 原来那个边缘节点减掉不能减的部分
//						perSiteModifyVal -= _optimMap.at(siteNode[siteID]).value[i][thisDemandID].first;
							int y = i;
							auto z = _optimMap.at(siteNode[siteID]).value;
							int x = *(_optimMap.at(siteNode[siteID]).value[i][thisDemandID]);
							int modifyResVal = perSiteModifyVal - *(_optimMap.at(siteNode[siteID]).value[i][thisDemandID]);
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] = 0;
							*optim[i][modifyDemand] -= modifyResVal;
							if(*optim[i][modifyDemand] < 0) {
								cout << "2" << endl;
							}
						}
					}
				}
				else { // 边缘节点剩余的带宽不够了，重新计算需要修改的量
					int thisSiteRes = siteNodeBandwidth[siteIndex] - siteSumTimeMap.at(i);
					int perSiteModifyValNow = floor(thisSiteRes / thisDemandUsableSiteVec.size());
					if(perSiteModifyValNow < 1) {
						continue;
					}
					*optim[i][modifyDemand] += perSiteModifyValNow * thisDemandUsableSiteVec.size();
					for (auto siteID: thisDemandUsableSiteVec) { // 遍历该客户节点可以修改的边缘节点
						string thisSiteName = siteNode[siteID];
						int thisDemandID = findDemandIndex(siteConnetDemand.at(thisSiteName), thisDemandName);
						if (*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] > perSiteModifyValNow) { // 够减
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] -= perSiteModifyValNow;
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << "3" << endl;
							}
						} else { // 不够减， 把能减的减了， 不能减的原来那个边缘节点减回去
//						perSiteModifyVal -= _optimMap.at(siteNode[siteID]).value[i][thisDemandID].first;
							int modifyResVal = perSiteModifyValNow - *_optimMap.at(siteNode[siteID]).value[i][thisDemandID];
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] = 0;
							*optim[i][modifyDemand] -= modifyResVal;
							if(*optim[i][modifyDemand] < 0) {
								cout << "4" << endl;
							}
						}
					}
				}
			}
//			else if(optim[i][modifyDemand].first == thisSiteAvr) continue;
			else { // 该时刻带宽量比平均值大, 减
				if (*optim[i][modifyDemand] >= val) { // 该边缘节点该时刻该客户节点有大于val的带宽量，减
					*optim[i][modifyDemand] -= perSiteModifyVal * thisDemandUsableSiteVec.size();
					// 其他边缘节点加
					for (const auto& siteID: thisDemandUsableSiteVec) {
						string thisSiteName = siteNode[siteID];
						int thisDemandID = findDemandIndex(siteConnetDemand.at(thisSiteName), thisDemandName);
						// 改, 应该是要修改的边缘节点的带宽总量 - 这个边缘节点在这个时刻花了多少
						if ((siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i)) > perSiteModifyVal) {
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] += perSiteModifyVal;
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << "5" << endl;
							}
						} else { // 该边缘节点该时刻该客户节点的带宽量比val小，则原边缘节点加上没减掉的量
                            assert(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] >= 0);
                            int z =  *_optimMap.at(siteNode[siteID]).value[i][thisDemandID];
                            int y = (siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i));
                            if (y < 0) {
                                cout << "de";
                            }
//							perSiteModifyVal -= (siteNodeBandwidth[siteID] - _optimMap.at(siteNode[siteID]).value[i][thisDemandID].first);
							int modifyResVal = perSiteModifyVal - (siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i));
							// 该边缘节点该时刻该客户节点加上能加的量
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] += (siteNodeBandwidth[siteID] -
									computeSiteSumTime(thisSiteName, i));
                            int x =  *_optimMap.at(siteNode[siteID]).value[i][thisDemandID];
                            assert(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] >= 0);
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << "6" << endl;
							}
							*optim[i][modifyDemand] += modifyResVal;
							if(*optim[i][modifyDemand] < 0) {
								cout << "7" << endl;
							}
						}
					}
				}
				else {// 该边缘节点该时刻该客户节点带宽量比val小，减少该带宽量
					int valNow = *optim[i][modifyDemand];
					int perSiteModifyValNow = floor(valNow / thisDemandUsableSiteVec.size());
					if(perSiteModifyValNow < 1) {
						continue;
					}
					*optim[i][modifyDemand] -= perSiteModifyValNow * thisDemandUsableSiteVec.size();
					if(*optim[i][modifyDemand] < 0) {
						cout << 11 << endl;
					}
					for (auto siteID: thisDemandUsableSiteVec) {
						string thisSiteName = siteNode[siteID];
						int thisDemandID = findDemandIndex(siteConnetDemand.at(thisSiteName), thisDemandName);
						if ((siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i)) > perSiteModifyValNow) {
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] += perSiteModifyValNow;
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << "8" << endl;
							}
						} else {
							int x = computeSiteSumTime(thisSiteName, i);
							int temp = siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i);
							int modifyResVal = perSiteModifyVal - (siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i));
							*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] += (siteNodeBandwidth[siteID] - computeSiteSumTime(thisSiteName, i));
							if(*_optimMap.at(siteNode[siteID]).value[i][thisDemandID] < 0) {
								cout << 9 << endl;
							}
							*optim[i][modifyDemand] += modifyResVal;
							if(*optim[i][modifyDemand] < 0) {
								cout << 10 << endl;
							}
						}
					}
				}
			}
		}

		modifiedSite.push_back(siteIndex);
	}
}

void Base::solve() {
	TicToc tictoc;
    solveFree();
	// 最大分配
	// 不断维护95%的带宽
	cout << getScore() << endl;
	int L = 4;
	while(L--) {
	 	updateBandwidth(L);
		uint32_t nowScore = getScore();
        // if (nowScore < bestScore) {
        //     goodResult = bestResult;
        //     bestScore = nowScore;
        // }
        cout << nowScore << endl;
		if(tictoc.toc() > 50000) break;
	}
    // cout << "best score : " << bestScore << endl;
}


void Base::_frameSiteOptim(unordered_map<string, Optim::DataType>& siteFrameLog) {
    for (auto& s: siteNode) {
        if (siteFrameLog.at(s).empty()) {
            continue;
        }
        _optimMap[s].value.emplace_back(siteFrameLog.at(s));
    }
}