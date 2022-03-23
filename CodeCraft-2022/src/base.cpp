#include <fstream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <assert.h>
#include <random>
#include "../inc/base.h"
#include "../inc/tic_toc.h"

using std::istringstream;
using std::ofstream;

using namespace std;

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

// void Base::solveMaxFree(){
//     vector<int> siteCnt(siteNode.size(), 0); //记录每个边缘节点使用的次数
// 	int maxFree = floor(demand.size()*0.05)-1; //向下取整，相当于向上取整取前5%，可以免费使用的每个边缘节点的次数
//     bool resOrNot = false;
//     for(auto demandFrame : demand){
//         vector<int> siteNodeBandwidthCopy(siteNodeBandwidth); //边缘节点带宽量的拷贝，每一轮拷贝一次
//         unordered_set<int> usedSiteIndex; //用于记录当前帧被最大分配使用的边缘节点的序号
//         for(size_t i = 0; i < demandFrame.size(); i++){
//             vector<pair<string, int>> preFramePreDemandResult; // 用于存储每一帧中的每一客户节点流量请求分发的结果
//             vector<int> demandUsableSiteIndex = usableSite.at(demandNode[i]); // 获取当前客户节点可用的边缘节点序号
//             if(judgeRestFree(demandUsableSiteIndex, siteCnt, maxFree)) { // 只要还有可以最大分配的边缘节点，先最大分配
//                     _largestMethod(demandFrame[i], siteNodeBandwidthCopy, demandUsableSiteIndex,
//                                 preFramePreDemandResult, siteCnt, maxFree, resOrNot, usedSiteIndex);
//         }
//     }
// }


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
void Base::solveAnne() {
	TicToc tictoc;

	int L = 20000; // 最大迭代次数
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
            if(j == _demandUsableSiteIndex.size() - 1) {
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
    auto it = record.cbegin();
    while (it != record.cend()) {
        pair<string, int> tmp(it->first, it->second);
        result.emplace_back(tmp);
        it++;
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

float computeErr(float x, float y) {
	return (x * x + y * y) * (x * x + y * y);
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
void updateWeight(Paramerter::Ptr weight, vector<int> demandUsableSiteIndex) {
	for (size_t i = 0; i < weight->value.size(); ++i) {
		// value的下标与可用边缘节点的下标相同, 遍历可用边缘节点
		// 计算该边缘节点的x, y
		float x = 0.0, y = 0.0;
		float e = computeErr(x, y);
		float a = computeAlpha(e);
		weight->value[i] *= a;
	}
	weight->softmax();
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
//			if(find(modifyLayers.begin(), modifyLayers.end(), i) != modifyLayers.end()and flag == false) {
//				updateWeight(*curLayer, demandUsableSiteIndex);
//			}
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

double randomVal(double min, double max) {
	uint64_t time = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 gen(time);
	std::uniform_real_distribution<double> dist(min, max);

	return dist(gen);
}


/**
 * @brief 为边缘节点生成一个时刻的随机权重
 * @param min
 * @param max
 * @param size
 * @return
 */
vector<double> randomWeight(int min, int max, int size) {
	vector<double> perFrameRandomWeight;
	uint64_t time = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 gen(time);
	std::uniform_real_distribution<double> dist(min, max);
	perFrameRandomWeight.reserve(size);
	for (int i = 0; i < size; ++i) {
		perFrameRandomWeight.emplace_back(dist(gen));
	}

	return perFrameRandomWeight;
}


void Base::solveWeight() {
	auto curLayer = _layers.begin();
	vector<vector<pair<string, int>>> resultNow;

	for (size_t i = 0; i < demand.size(); ++i) {
		auto demandFrame = demand[i];
		auto dSize = demandNode.size();
		vector<int> siteNodeBandwidthCopy = siteNodeBandwidth;
		for (size_t j = 0; j < demandFrame.size(); ++j) {
			vector<pair<string, int>> perFramePerDemandResult;
			vector<int> demandUsableSiteIndex = usableSite.at(demandNode[j]);
			_weightMethod(demandFrame[j], siteNodeBandwidthCopy, demandUsableSiteIndex,
						  perFramePerDemandResult, *curLayer);
			resultNow.emplace_back(perFramePerDemandResult);
			++curLayer;
		}
	}

	result = resultNow;
}

/**
 * @brief 选择不同方法进行流量分配
 * @param name max or weight
 */
void Base::_bandAllocate(const string& name) {
	if(name == "Max") {
		solveMaxAndWeight();
	}
	else if(name == "Weight") {
		solveWeight();
	}
	else{
		cerr << "name must be Max or weight" << endl;
		return;
	}
}

vector<pair<double, double>> computeAverage(unordered_map<string, Optim> _optimMap, vector<string> siteNode, int maxFree) {
	vector<pair<double, double>> siteAvrVec;
	for(auto & node : siteNode) { // 遍历边缘节点
		auto siteMap = _optimMap.at(node).value;
		sort(siteMap.begin(), siteMap.end(), [](Optim::DataType a, Optim::DataType b) {return a.first > b.first;});
		double sum5 = 0.0, sum95 = 0.0;
		for (int i = 0; i < maxFree; ++i) {
			sum5 += siteMap[i].first;
		}
		double avr5 = sum5 / maxFree;
		for (int i = maxFree; i < siteMap.size(); ++i) {
			sum95 += siteMap[i].first;
		}
		double avr95 = sum95 / siteMap.size();
		pair<double, double> perSiteAvrVec(avr5, avr95);
		siteAvrVec.emplace_back(perSiteAvrVec);
	}

	return siteAvrVec;
}

double computeFitness(double x_hat, double y_hat) {
	double fitness = 0.3 * x_hat * x_hat - 0.3 * y_hat * y_hat + 0.4 * (x_hat - y_hat) * (x_hat - y_hat);

	return fitness;
}

//void Base::_differentialEvoMethod(unordered_map<string, Optim> _optimMap, size_t Gm) {
//	// 初始化参数
//	size_t NP = siteNode.size(); // 种群数量
//	size_t G = 2000; // 最大迭代次数
//	double lam = exp(1 - (float)Gm / (float)(Gm + 1 - G));
//	double F = 0.5 * pow(2, lam);
//	double CR = 0.1 * (1 + randomVal(0, 1)) * F;
//
//	vector<pair<double, double>> siteAvrVec = computeAverage(_optimMap, siteNode, maxFree); // 边缘节点的5，95平均值
//	for(auto & node : siteNode) { // 遍历边缘节点
//		// 变异
////		auto siteWeight = _optimMap[node].value;
//		auto siteConnectIndex = siteConnectDemand.at(node);
//		// 产生一个随机变异个体
//		int weightSize = siteConnectIndex.size();
//		vector<vector<double>> perSiteWeight; // 保存了一个边缘节点
//		for (size_t i = 0; i < demand.size(); ++i) {
//			auto perSitePerFrameWeight = randomWeight(0, 1, weightSize);
//			perSiteWeight.emplace_back(perSitePerFrameWeight);
//		}
//
//		// 交叉
//		// 每个时刻都要进行一次交叉
//		for (int i = 0; i < demand.size(); ++i) {
//			for (int j = 0; j < siteConnectIndex.size(); ++j) { // 遍历连接的客户节点
//				double ran = randomVal(0, 1);
//				if(ran < CR) {
//					_optimMap.at(node).value[i * siteConnectIndex.size() + j].second = perSiteWeight[i][j];
//				}
//			}
//		}
//	}
//
//
//
//}


void Base::solve() {
	maxFree = floor(demand.size() * 0.05);

	// 先分配

	// 再进化
}
