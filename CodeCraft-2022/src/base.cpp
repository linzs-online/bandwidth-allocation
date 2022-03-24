#include <fstream>
#include <sstream>
#include <algorithm>
#include <list>
#include <math.h>
#include <assert.h>
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
    log = std::make_shared<SiteLog>(siteNode, siteNodeBandwidth, demand.size());
	// 找到满足qos的site
	findUsableSite();
    // 参数初始化
    paramInit();

    maxFree = floor(demand.size() * 0.05) - 1;
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
        _optimMap.emplace(std::move(siteNoteName), std::move(Optim()));
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
        size_t demandSum = 0;
        while(getline(sin, temp, ',')){
            size_t nodeDemand = atoi(temp.c_str());
            demandSum += nodeDemand;
            demandFrame.emplace_back(nodeDemand);
        }
        demand.emplace_back(demandFrame); //将所有时间序列的客户节点请求封装成vector
        pair<size_t,size_t>  _temp(demandSum,frameID);
        frameDmandSumMap.insert(_temp);
        frameID++; //帧ID++，为下一帧处理准备
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
        l->value.assign(number, 0);
        l->init(number);
        l->softmax();
        _layers.emplace_back(std::move(l));
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

// void Base::solveMaxFree(){
//     vector<int> siteCnt(siteNode.size(), 0); //记录每个边缘节点使用的次数
// 	int maxFree = floor(demand.size()*0.05)-1; //向下取整，相当于向上取整取前5%，可以免费使用的每个边缘节点的次数
//     bool resOrNot = false;
//     for(auto demandFrame : demand){
//         vector<int> siteNodeBandwidthCopy = siteNodeBandwidth; //边缘节点带宽量的拷贝，每一轮拷贝一次
//         unordered_set<int> usedSiteIndex; //用于记录当前帧被最大分配使用的边缘节点的序号
//         for(size_t i = 0; i < demandFrame.size(); i++){
//             vector<pair<string, int>> preFramePreDemandResult; // 用于存储每一帧中的每一客户节点流量请求分发的结果
//             vector<int> demandUsableSiteIndex = usableSite.at(demandNode[i]); // 获取当前客户节点可用的边缘节点序号
//             if(judgeRestFree(demandUsableSiteIndex, siteCnt, maxFree)){ // 只要还有可以最大分配的边缘节点，先最大分配
//                     _largestMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex,
//                                 preFramePreDemandResult, siteCnt, maxFree, resOrNot, usedSiteIndex);
//                 if(resOrNot){ // 有未被分配的带宽，该轮demand还需要平均分配
//                     maxDeal(demandFrame[i], siteNodeBandwidthCopy,
//                             demandUsableSiteIndex, preFramePreDemandResult);
//                     resOrNot = false;
//                 }
//             }
//             else{ //所有边缘节点免费使用次数白嫖完成，剩下的流量请求使用最大流分配的方式
//                 maxDeal(demandFrame[i], siteNodeBandwidthCopy,
//                         demandUsableSiteIndex, preFramePreDemandResult);
//             }
//             result.push_back(preFramePreDemandResult); //当前帧的当前客户节点流量请求分配方案存入结果
//         }
//         //一帧的所有客户节点的流量请求处理完成，对在最大分配方案中使用过的边缘节点记录一次
//         for(auto& it : usedSiteIndex){
//             siteCnt[it]++;
//         }
//     }
// }

// void Base::solveMaxFree(){
//     //unordered_map<string,int> siteUsedCnt;
//     //首先初始化每个边缘节点的使用次数,都是0
//     for(auto& sName : siteNode){
//         pair<string,int> _t(sName,0);
//         siteUsedCnt.insert(_t);
//     }
//     list<string> siteSelectedList; //升序排列，最后一个元素中可以连上的客户最多
//     for(auto it = siteConnetDemandSize.begin(); it != siteConnetDemandSize.end(); it++){
//         siteSelectedList.push_back(it->second);
//     }
//     list<string> freeSite(siteSelectedList); //存储还有免费使用次数的列表
//     //一帧一帧处理，这里用反向迭代器，先处理最大请求量大的帧
//     for(auto idPtr = frameDmandSumMap.rbegin(); idPtr != frameDmandSumMap.rend(); idPtr++){
//         list<string> siteListCopy(siteSelectedList); //可用的边缘节点初始化
//         unordered_map<string,int> siteBandWithCopy(siteBandWith);  //每帧的边缘节点带宽初始化
//         unordered_set<string> usedSiteName; //用于记录当前帧被最大分配使用的边缘节点的名字
//         size_t frameID = idPtr->second; //当前要处理的帧ID号，用于最后存入结果
//         // 客户 - 请求结果
//         unordered_map<string,vector<pair<string,int>>> perFrameResult; //存储当前帧的带宽分发结果，用于最后存入结果
//         unordered_map<string,size_t> frame; // 客户节点名字-它的请求流量
//         for(auto i =0; i < demand[frameID].size(); i++){ //把一帧的各个客户请求取出来
//             pair<string,size_t> _temp(demandNode[i], demand[frameID][i]);
//             frame.insert(_temp); // 现在当前帧的流量请求以及以 客户名字-请求流量大小 的方式存储
//             vector<pair<string,int>> perDresult;
//             pair<string,vector<pair<string,int>>> Fresult(demandNode[i],perDresult);
//             perFrameResult.insert(Fresult); //创建一个每帧结果存储器，等待结果按照客户名字索引放入
//         }
//         while(!freeSite.empty()){
//             freeMode(frame, freeSite, siteBandWithCopy, perFrameResult, usedSiteName);
//         }
//         if(!frame.empty()){
//             averMode(frame, siteListCopy, siteBandWithCopy, perFrameResult);
//         }
//         for(auto& name : usedSiteName){
//             siteUsedCnt.at(name)++;
//         }
//     }
// }

void Base::solveMaxFree(){
    //unordered_map<string,int> siteUsedCnt;
    //首先初始化每个边缘节点的使用次数,都是0
    for(auto& sName : siteNode){
        pair<string,int> _t(sName,0);
        siteUsedCnt.insert(_t);
    }
    list<string> siteSelectedList; //降序排列，最后一个元素中可以连上的客户最少
    for(auto it = siteConnetDemandSize.rbegin(); it != siteConnetDemandSize.rend(); it++){
        siteSelectedList.push_back(it->second);
        vector<uint32_t> _temp;
        pair<string, vector<uint32_t>> _Temp(it->second,_temp);
        fixedTime.insert(_Temp);
    }
    list<string> freeSite(siteSelectedList); //存储还有免费使用次数的边缘节点的列表
    //一帧一帧处理，这里用反向迭代器，先处理最大请求量大的帧
    for(auto idPtr = frameDmandSumMap.rbegin(); idPtr != frameDmandSumMap.rend(); idPtr++){
        uint32_t frameID = idPtr->second; //当前要处理的帧ID号，用于最后存入结果
        unordered_map<string,vector<pair<string,int>>> perFrameResult; //存储当前帧的带宽分发结果，用于最后存入结果
        unordered_map<string,int> siteBandWithCopy(siteBandWith);  //每帧的边缘节点带宽初始化
        unordered_set<string> usedSiteName; //用于记录当前帧被使用的边缘节点的名字
        unordered_map<string,uint64_t> frameDmandMap; // 客户节点名字-请求流量
        vector<int> frameDemand(demand[frameID]);
        for(auto i =0; i < demand[frameID].size(); i++){ //把一帧的各个客户请求取出来
            pair<string,uint64_t> _temp(demandNode[i], demand[frameID][i]);
            frameDmandMap.insert(_temp); // 现在当前帧的流量请求以及以 请求流量大小-客户名字 的方式存储
            vector<pair<string,int>> perDresult;
            pair<string,vector<pair<string,int>>> Fresult(demandNode[i],perDresult);
            perFrameResult.insert(Fresult); //创建一个每帧结果存储器，等待结果按照客户名字索引放入
        }
        auto theMapSum = [&](unordered_map<string,uint64_t> demandMap)->uint64_t{
            uint64_t sum = 0;
            for(auto it = demandMap.begin(); it != demandMap.end(); it++){
                sum += it->second;
            }
            return sum;
        };
        uint64_t totalDemand = theMapSum(frameDmandMap);
        while (theMapSum(frameDmandMap) > 0)
        {
            if(!freeSite.empty()){
                for(auto site : freeSite){
                    uint64_t totalComsume = 0;
                    if(siteBandWithCopy.at(site) == 0) continue; //这个边缘节点的带宽没了，跳过找下一个
                    if(theMapSum(frameDmandMap) == 0) break; //这一帧的所有请求已经满足，跳过检索，准备处理结果
                    for(auto demand_it = siteConnetDemand.at(site).begin(); demand_it != siteConnetDemand.at(site).end(); demand_it++){
                        if(frameDmandMap.at(*demand_it) == 0) continue; //加速，防止反复检查
                        // 如果第一个客户的请求量比这个边缘节点的带宽还要大
                        if(frameDmandMap.at(*demand_it) > siteBandWithCopy.at(site)){
                            frameDmandMap.at(*demand_it) -= siteBandWithCopy.at(site);
                            pair<string,int> _temp(site, siteBandWithCopy.at(site));
                            perFrameResult.at(*demand_it).push_back(_temp);
                            totalComsume += siteBandWithCopy.at(site);
                            siteBandWithCopy.at(site) = 0;
                            break;
                        }
                        else{ //这个边缘节的带宽比一个客户的请求量还大
                            siteBandWithCopy.at(site) -= frameDmandMap.at(*demand_it);
                            pair<string,int> _temp(site, frameDmandMap.at(*demand_it));
                            perFrameResult.at(*demand_it).push_back(_temp);
                            totalComsume += frameDmandMap.at(*demand_it); //边缘节点的消耗叠加
                            frameDmandMap.at(*demand_it) = 0;
                            if(siteBandWithCopy.at(site) == 0) break;
                        }
                    }
                    float rate = float(totalComsume) / totalDemand;
                    if(rate > 0.7 || (float(totalComsume) / siteBandWith.at(site)) > 0.9){
                        usedSiteName.insert(site);
                        fixedTime.at(site).push_back(frameID);
                    }
                }
            }
            if(theMapSum(frameDmandMap) > 0){
                for(auto& dmandMap : frameDmandMap){
                    if(dmandMap.second == 0) continue;
                    vector<int> canUseSite = usableSite.at(dmandMap.first);
                    Paramerter::Ptr w = std::make_shared<Paramerter>();
                    vector<pair<string, int>> resResult;
                    w->value.assign(canUseSite.size(), 1.0f / canUseSite.size());
                    _weightMethod(dmandMap.second, siteBandWithCopy, canUseSite, resResult, w);
                    frameDmandMap.at(dmandMap.first) = 0;
                    for(auto& _r : resResult){
                        perFrameResult.at(dmandMap.first).push_back(_r);
                    }
                }
            }
        }
        //结果处理
        vector<vector<pair<string,int>>> resutl;
        for(auto& demandName : demandNode){
            resutl.push_back(perFrameResult.at(demandName));
        }
        pair<uint32_t,vector<vector<pair<string,int>>>> _temp(frameID,resutl);
        outputResult.insert(_temp);
        // 每帧结束之前把在当前帧使用过的边缘节点的次数++
        for(auto& name : usedSiteName){
            siteUsedCnt.at(name)++;
        }
        for(auto cnt : siteUsedCnt){
            if(cnt.second > maxFree) continue;  // 防止反复删除
            if(cnt.second == maxFree){
                freeSite.remove(cnt.first);
            }
        }
    }
    for(auto& _preFrameResult : outputResult){
        for(auto& _result : _preFrameResult.second){
            result.push_back(_result);
        }
    }
}

void Base::averageMode(pair<string, uint64_t> demandNow, 
                        vector<pair<string, uint64_t>> siteBWCopy,
                        vector<pair<string,uint64_t>> result){
    for(auto i =0; i < usableSite.at(demandNow.first).size(); i++){
        
    }
}
// void Base::freeMode(unordered_map<string,size_t>& frame, 
//                     list<string>& siteCanUse,
//                     unordered_map<string,int>& siteBandWithCopy,
//                     unordered_map<string,vector<pair<string,int>>>& perFrameResult,
//                     unordered_set<string>& usedSiteName){
//     //把边缘节点能连接上的客户节点的名字取出放到vector里面，先处理这部分节点
//     vector<string> preDealDemandNode;
//     preDealDemandNode = siteConnetDemand.at(siteCanUse.back());
//     size_t demandSum = 0; //先算一下这些客户请求的总量
//     for(auto& _name : preDealDemandNode){
//         demandSum += frame[_name];
//     }
//     if(siteBandWithCopy.at(siteCanUse.back()) >= demandSum){
//         //当这个边缘节点带宽总量比能连上的所有客户请求总量还大的时候，直接满足所有请求
//         for(auto& _name : preDealDemandNode){
//             pair<string,int> _temp(siteCanUse.back(), frame.at(_name));
//             perFrameResult.at(_name).emplace_back(_temp);//存入已经被满足的客户请求结果
//             frame.erase(_name);  //被满足的节点全部擦除
//         }
//         siteBandWithCopy.at(siteCanUse.back()) -= demandSum; //返回边缘节点的余量
//     }
//     else{
//         //当这个边缘节点带宽总量比能连上的所有客户请求总量小的时候，优先满足请求最大的那个,直到分配完这个边缘节点的带宽
//         map<size_t,string> demandNow;
//         for(auto it = frame.begin(); it != frame.end(); it++){
//             pair<size_t,string> _temp(it->second,it->first);
//             demandNow.insert(_temp); // （升序排列）存储形式：需求量 - 客户节点名字 
//         }
//         while(siteBandWithCopy.at(siteCanUse.back()) > 0){
//             if(siteBandWithCopy.at(siteCanUse.back()) < demandNow.rbegin()->first){
//                 //最大那个请求就可以把这个边缘节点的带宽消耗完毕
//                 frame.at(demandNow.rbegin()->second) -= siteBandWithCopy.at(siteCanUse.back());
//                 siteBandWithCopy.at(siteCanUse.back()) = 0;
//                 pair<string,int> _temp(siteCanUse.back(),siteBandWithCopy.at(siteCanUse.back()));
//                 perFrameResult.at(demandNow.rbegin()->second).emplace_back(_temp);
//                 usedSiteName.insert(siteCanUse.back());
//                 siteCanUse.pop_back(); //在可用列表中删除它
//             }
//             else{ //这个边缘节点在满足最大的客户请求之后还能继续分配
//                 siteBandWithCopy.at(siteCanUse.back()) -= frame.at(demandNow.rbegin()->second);
//                 pair<string,int> _temp(siteCanUse.back(),frame.at(demandNow.rbegin()->second));
//                 perFrameResult.at(demandNow.rbegin()->second).emplace_back(_temp);
//                 //满足了请求之后在请求表中删除这个客户请求
//                 frame.erase(demandNow.rbegin()->second);
//                 auto it = demandNow.end();
//                 demandNow.erase(--it);
//             }
//         }
//     }
// }


void Base::maxDeal(int& demandNow,
            vector<int>& siteNodeBand, 
            const vector<int>& demandUsableSite, 
            vector<pair<string, int>>& perResult){
    for(auto j = 0; j < demandUsableSite.size(); j++){
        //如果当前边缘节点带宽小于流量请求,那就把这个边缘节点全部分配给这个请求
        if(demandNow >= siteNodeBand[demandUsableSite[j]]){
            if(siteNodeBand[demandUsableSite[j]] == 0) continue;
            demandNow -= siteNodeBand[demandUsableSite[j]];
            pair<string,int> _temp(siteNode[demandUsableSite[j]],siteNodeBand[demandUsableSite[j]]);
            perResult.emplace_back(_temp);
            siteNodeBand[demandUsableSite[j]] = 0;
        }
        else if(demandNow < siteNodeBand[demandUsableSite[j]])
        { //当前请求小于边缘节点带宽，把边缘节点的部分带宽切给请求
            siteNodeBand[demandUsableSite[j]] -= demandNow;
            pair<string,int> _temp(siteNode[demandUsableSite[j]],demandNow);
            perResult.emplace_back(_temp);
            demandNow = 0;
            break;
        }
    }
}


void Base::solveMaxAndWeight() {
	vector<int> siteCnt(siteNode.size(), 0); //记录每个边缘节点使用的次数
	int maxFree = floor(demand.size()*0.05)-1; //向下取整，相当于向上取整取前5%，可以免费使用的每个边缘节点的次数
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
 * @brief 模拟退火版solve
 */
void Base::solve() {
	TicToc tictoc;

	int L = 10; // 最大迭代次数
	float T = 1.0;
	float endT = 1e-16;
	bool flag = true; // 用作代表第一次迭代
	while (--L) {
		simulatedAnnealing(flag, T);
		if(T < endT)
        {
            cout << "已达最大迭代次数" << endl;
            break;
        } 
        if(tictoc.toc() > 250000) break;  //达到最大时间限制，终止
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
            if(j == _demandUsableSiteIndex.size() - 1){
                flag = true;
            }
        }
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
 * @brief 权重策略进行分配
 * @param demanNow 某个客户需求量
 */
void Base::_weightMethod(int demandNow,
            vector<int>& siteNodeBand, 
            const vector<int>& demandUsableSite, 
            vector<pair<string, int>>& result,
            Paramerter::Ptr weight) {
    unordered_map<string, int> record;

    _weightDistribution(demandNow, demandNow, siteNodeBand, 
                demandUsableSite, record, weight);
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        int site = demandUsableSite[i];
        int band = _totalBand(siteNode[site], record);
        // 权重记录
        Optim::DataType data(band, weight->value[i]);
        _optimMap.at(siteNode[site]).value.emplace_back(std::move(data));
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
    for (auto& r: record) {
        if (r.second != 0) {
            pair<string, int> tmp(r.first, r.second);
            result.emplace_back(move(tmp));
        }
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
        if (siteNodeBand[site] == 0 || weight->value[i] == 0.0f) {
            continue;
        }
        assert(!isnan(weight->value[i]));
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

float computeErr(float x, float y) {
	return (x * x * x + y * y * y);
}

float computeAlpha(float err) {
	return exp(-err);
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


/**
 * @brief 更新一个客户节点与其相连的边缘节点的权重
 * @param weight 该客户节点与边缘节点的权重
 *
 */
void Base::updateWeight(Paramerter::Ptr weight, vector<int> demandUsableSiteIndex, size_t frameId) {
	for (size_t i = 0; i < weight->value.size(); ++i) {
		// value的下标与可用边缘节点的下标相同, 遍历可用边缘节点
		// 计算该边缘节点的x, y
        size_t times = log->siteLogMap.at(siteNode[demandUsableSiteIndex[i]]).first; 
        auto usageRate = log->siteLogMap.at(siteNode[demandUsableSiteIndex[i]]).second;
        double usage = accumulate(usageRate.begin(), usageRate.end(), 0.0);
        float rana = (rand()%3 - 1) * 0.5;
		float x = (float)times / (float)frameId;
        // float x = 0.0;
        // if(times <= maxFree) {
        //     x = 0.2 + ran;
        // }
        // else{
        //     x = 0.8 + ran;
        // }
        size_t ran = (rand()% 2);
        float y = usage / ((float)times + 1e-10);
		float e = computeErr(x, y);
		float a = computeAlpha(e);
        // cout << weight->value[i] << " ";
		weight->value[i] += a * 10;
        if (weight->value[i] < 0.0f) {
            weight->value[i] = 0.0f;
        }
	}
	weight->softmax();

    // cout << "1----------------------1" << endl;
    // for (size_t i = 0; i < weight->value.size(); i++)
    // {
    //     cout << weight->value[i] << " ";
    // }

    // cout << "2-----------------------------2" << endl;
    
}


/**
 * @brief 模拟退火算法实现
 *
 */
void Base::simulatedAnnealing(bool &flag, float &T) {
	float dT = 0.99;
	vector<vector<pair<string, int>>> resultNow;
	auto curLayer = _layers.begin();
	// auto modifyLayers = randomLayer(0, demand.size() - 1, demand.size() * 0.3);
    
    log->logClear();
	
    for(size_t i = 0; i < demand.size(); ++i) { //
		auto demandFrame = demand[i];
        auto dSize = demandNode.size();
		vector<int> siteNodeBandwidthCopy = siteNodeBandwidth; // 边缘节点带宽量的拷贝，每一时刻拷贝一次
		for (size_t j = 0; j < demandFrame.size(); ++j) {
			vector<pair<string, int>> preFramePreDemandResult; // 当前时刻当前客户节点的分配方案
			vector<int> demandUsableSiteIndex = usableSite.at(demandNode[j]); // 当前客户节点连接的边缘节点
			_weightMethod(demandFrame[j], siteNodeBandwidthCopy, demandUsableSiteIndex,
						  preFramePreDemandResult, *curLayer);                          
            // log->write2Log(preFramePreDemandResult, i); 
			// 对当前层的客户节点权重进行更新
			// if(find(modifyLayers.begin(), modifyLayers.end(), i) != modifyLayers.end() and flag == false) {
			// 	updateWeight(*curLayer, demandUsableSiteIndex, i + 1);
			// }
			resultNow.emplace_back(preFramePreDemandResult);
			++curLayer;
		}
	}
    _updateAllWeight();
	if(flag) {
		// 第一次迭代
		result = resultNow;
		flag = false;
	}
	else{ // 比较成本
		auto scoreNow = getScore(resultNow);
		auto scoreLast = getScore(result);
		float df = scoreNow - scoreLast;

		double p = rand()%10000;     //p为0~1之间的随机数
		if(df < 0) {
			result = resultNow;
		}
		else if(exp(-df / T) > p) {
			result = resultNow;
		}
	}
    cout << "当前迭代得分: " << getScore(resultNow) << endl;
	T *= dT;
}


/**
 *@brief 更新所有权重 
 */
void Base::_updateAllWeight() {
    for (auto& site: siteNode) {
        Optim& op = _optimMap.at(site);
        if (op.value.empty()) {
            continue;
        }
        int mean = op.mean();
        op.step(mean);
    }
    for (auto& l: _layers) {
        l->softmax();
    }
}