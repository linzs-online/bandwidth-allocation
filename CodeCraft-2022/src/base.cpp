#include <fstream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include "../inc/base.h"

using std::istringstream;
using std::ofstream;

string temp,line;


Base::Base(string&& _filePath){
    // 初始化siteNodo（ ）
    siteNodeInit(_filePath + "site_bandwidth.csv");
    // 初始化demand（）
    demandNodeInit(_filePath + "demand.csv");
    // 初始化QOS()
    qosInit(_filePath + "qos.csv");
    // 初始化config（）
    qosConstraintInit(_filePath + "config.ini");
	// 找到满足qos的site
	findUsableSite();
    // 参数初始化
    paramInit();
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
    while(getline(demandfile,line)){
        istringstream sin(line);
        getline(sin, temp, ',');
        while(getline(sin, temp, ',')){
            demandFrame.emplace_back(atoi(temp.c_str()));
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
unsigned int Base::getScore(vector<vector<pair<string, int>>>& result){
    vector<unordered_map<string,vector<pair<string, int>>>> preFrameResult;
    this->getPreFrameResult(preFrameResult, result);
    unordered_map<string,vector<unsigned int>> siteNodeConsume;
    //vector<vector<pair<string, int>>> result;
    for(auto _siteNodeName : siteNode){
        vector<unsigned int> _Vtemp(preFrameResult.size(),0);  //直接指定每个边缘节点的消耗历史记录表的大小
        pair<string,vector<unsigned int>> _temp(_siteNodeName,_Vtemp);
        siteNodeConsume.insert(_temp);  //初始化一个unordermap用来存放每个边缘节点的消耗历史
    }
    int frameInedx = 0;
    for(auto& frame : preFrameResult){
        for(auto& dNodeResult: frame){ //每帧中，每个客户节点对应的边缘节点的分配方案
            for(auto& pair : dNodeResult.second){ //把分配方案中边缘节点的流量分发汇总
                siteNodeConsume.at(pair.first)[frameInedx] += pair.second;
            }
        }
        frameInedx++; //一帧的数据处理完成了，把帧序号递增
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
        l->softmax();
        _layers.emplace_back(l);
    }
}


void Base::save(string&& _fileName) {
    ofstream file(_fileName, std::ios_base::out);
    int customerNum = demandNode.size();
    for(size_t i = 0; i < result.size(); ++i) {
        int curCustomerID = i % customerNum;
        // 客户ID
        file << demandNode[curCustomerID] << ":";
        if (result[i].empty() || result[i][0].second == 0) {
            if (i < result.size() - 1) {
                file << "\n";
            }
            continue;
        } 
        // 分配方案
        for (size_t j = 0; j < result[i].size(); ++j) {
            file << "<" << result[i][j].first << "," 
                << result[i][j].second << ">";
            if (j == result[i].size() - 1) {
                if (i < result.size() - 1) {
                    file << "\n";
                }
            } else {
                file << ",";
            }
        }
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


void Base::solve() {
	vector<int> siteCnt(siteNode.size(), 0); //记录每个边缘节点使用的次数
	int maxFree = floor(demand.size() * 0.05) - 1; //向下取整，相当于向上取整取前5%，可以免费使用的每个边缘节点的次数
	bool flag = false; // 最大分配后是否还有带宽未被分配
    auto curLayer = _layers.begin();
    for(auto demandFrame : demand){  //从请求列表中一帧一帧取出来
        vector<int> siteNodeBandwidthCopy = siteNodeBandwidth; //边缘节点带宽量的拷贝，每一轮拷贝一次
        unordered_set<int> usedSiteIndex; //用于记录当前帧被最大分配使用的边缘节点的序号
        for(size_t i = 0; i < demandFrame.size(); ++i){ // 遍历一帧中所有的客户节点的流量请求
            vector<pair<string, int>> preFramePreDemandResult; // 用于存储每一帧中的每一客户节点流量请求分发的结果
            vector<int> demandUsableSiteIndex = usableSite.at(demandNode[i]); // 获取当前客户节点可用的边缘节点序号
//			_largestMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex, resultCustom, siteCnt, maxFree, flag);
            // _weightMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex, 
            //                 preFramePreDemandResult, *curLayer);
			if(judgeRestFree(demandUsableSiteIndex, siteCnt, maxFree)) { // 只要还有可以最大分配的边缘节点，先最大分配
				_largestMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex, 
                                preFramePreDemandResult, siteCnt, maxFree, flag, usedSiteIndex);
                if(flag) { // 有未被分配的带宽，该轮demand还需要平均分配
					_weightMethod(demandFrame[i], siteNodeBandwidthCopy, 
                                demandUsableSiteIndex, preFramePreDemandResult, *curLayer);
					flag = false;
				}
			}
			else{ //所有边缘节点免费使用次数白嫖完成，剩下的流量请求使用平均分配的方式
				_weightMethod(demandFrame[i], siteNodeBandwidthCopy, 
                        demandUsableSiteIndex, preFramePreDemandResult, *curLayer);
			}
            curLayer++;
            result.push_back(preFramePreDemandResult); //当前时刻当前客户节点流量请求分配方案存入结果
        }
        //一帧的所有客户节点的流量请求处理完成，对在最大分配方案中使用过的边缘节点记录一次
        for(auto& it : usedSiteIndex){
            siteCnt[it]++;
        }
    }
}



/**
 * @brief 最大划分
 * @param demanNow 某个客户的流量请求
 * @param siteNodeBand 边缘节点带宽量
 * @param demandUsableSite 延迟满足的边缘节点
 * @param result 结果
 */
//_largestMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex, resultCustom, siteCnt, maxFree, flag);
void Base::_largestMethod(int &demandNow,
            vector<int>& siteNodeBand,
            const vector<int>& _demandUsableSiteIndex,
            vector<pair<string, int>>& _preFramePreDemandResult,
			vector<int> &siteCnt,
			const int maxFree,
			bool &flag,
            unordered_set<int>& usedSiteIndex) const {
    for(size_t j = 0; j < _demandUsableSiteIndex.size(); ++j) { // 遍历可用边缘节点
		if(siteCnt[_demandUsableSiteIndex[j]] >= maxFree){
            //cout << "该客户节点可以使用的这个边缘节点的免费使用次数已达最大,将检索下一个可用的边缘节点" << endl;
			if(j == _demandUsableSiteIndex.size() - 1) { // 遍历边缘节点时出现了一直跳过的情况
				flag = true;
                //cout << "所有边缘节点使用次数已达最大,将采用平均分配策略" << endl;
			}
			continue;
		}
        if(siteNodeBand[_demandUsableSiteIndex[j]] == 0) {
            //检索到的当前边缘节点剩余带宽为0，将跳过，检索下一个可用的边缘节点
			if(j == _demandUsableSiteIndex.size() - 1) { // 有带宽没分完
				flag = true;
			}
            continue;
        }
        if(demandNow <= siteNodeBand[_demandUsableSiteIndex[j]]) {   // 如果当前流量请求小于当前边缘节点的带宽                                                            // 若边缘节点可以分配足够带宽
            siteNodeBand[_demandUsableSiteIndex[j]] -= demandNow; // 该边缘节点的带宽减去流量请求，存下剩余带宽
            pair<string, int> distributionRes(siteNode[_demandUsableSiteIndex[j]], demandNow);
            _preFramePreDemandResult.push_back(distributionRes); //因为流量请求小于当前边缘节点的带宽，所以分配一个边缘节点部分带宽即够
			usedSiteIndex.insert(_demandUsableSiteIndex[j]);
			demandNow = 0;  //已经满足当前的流量请求
            break;
        }
        else{   // 单个边缘节点带宽量不够
            demandNow -= siteNodeBand[_demandUsableSiteIndex[j]]; // 将剩余的带宽量全部分配给当前请求
            pair<string, int> x(siteNode[_demandUsableSiteIndex[j]], siteNodeBand[_demandUsableSiteIndex[j]]);
            siteNodeBand[_demandUsableSiteIndex[j]] = 0; //分配完之后该边缘节点的剩余带宽已经为0
            _preFramePreDemandResult.push_back(x); //把当前边缘节点的全部带宽分配到该请求作为结果存入
			usedSiteIndex.insert(_demandUsableSiteIndex[j]); //存入使用过的边缘节点的序号
            if(j == _demandUsableSiteIndex.size() - 1) {
                flag = true;
            }
        }
    }
}

/**
 * @brief 权重策略进行分配
 * @param demanNow 某个客户需求量
 */
void Base::_weightMethod(int demandNow,
            vector<int>& siteNodeBand, 
            const vector<int>& demandUsableSite, 
            vector<pair<string, int>>& result,
            Paramerter::Ptr weight) const {
    unordered_map<string, int> record;
    _weightDistribution(demandNow, demandNow, siteNodeBand, 
                demandUsableSite, record, weight);
    auto it = record.cbegin();
    while (it != record.cend()) {
        pair<string, int> tmp(it->first, it->second);
        result.emplace_back(tmp);
        it++;
    }
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
 * @brief 更新record, weight
*/
static inline bool _updateRecord(
            Paramerter::Ptr weight,
            const int id,
            unordered_map<string, int>& record, 
            const string& key,
            const int totalDemand,
            const int band) {
    int planValue = _totalBand(key, record);
    int reactValue = planValue + band;
    float oldWeight = weight->value[id];
    record[key] = reactValue;
    // 仅在边缘节点无法满足计划是分配
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
        const int demandNow) {
    uint32_t sum = 0;
    for (size_t i = 0; i < weight->value.size(); ++i) {
        sum += demandNow * weight->value[i];
    }
    return demandNow - sum;  
}

void Base::_weightDistribution(int demandNow,
                const int totalDemand,
                vector<int>& siteNodeBand, 
                const vector<int>& demandUsableSite,
                unordered_map<string, int>& record,
                Paramerter::Ptr weight) const {
    // 权重策略
    const uint demandInitial = demandNow;
    int res = _getRes(weight, demandNow);
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        
        if (demandNow == 0) {
            return; //递归终止
        }
        int site = demandUsableSite[i];
        if (siteNodeBand[site] == 0 || weight->value[i] == 0.0f) {
            continue;
        }
        int allocDemand = demandInitial * weight->value[i];
        // 获得边缘节点分配量
        int band = 0;
        auto getBand = [&](int* demand, bool is_res) {
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
            if (is_res) {
                *demand -= (band - tmp);
            }

        };
        getBand(&allocDemand, false);
        getBand(&res, true);
        if (band == 0) {
            continue;
        }
        // 更新
        demandNow -= band;
        bool update = _updateRecord(weight, i, record, siteNode[site], 
                        totalDemand, band);
        if (update) {
            // 余数调整
            res = _getRes(weight, demandInitial);
        }
    }
    // 递归分配余量
    _weightDistribution(demandNow, totalDemand,siteNodeBand,
            demandUsableSite, record, weight);
}


/**
 * @brief 判断是边缘节点中否还有免费使用的次数
 * 
 * @param siteCnt 
 * @param _maxFree 
 * @return true 
 * @return false 
 */
bool Base::judgeRestFree(vector<int>& _demandUsableSiteIndex ,vector<int>& _siteCnt, int _maxFree){
		for(auto& index : _demandUsableSiteIndex){
			if( _siteCnt[index] <= _maxFree){
				return true;
			}
		}
    return false;
}