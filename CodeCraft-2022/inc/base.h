#pragma once

#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>
#include "paramter.h"

using namespace std;
using std::vector;
using std::string;
using std::unordered_map;
using std::pair;
using std::unordered_set;

class Base{
private:
    vector<string> demandNode;
    unordered_map<string,int> siteBandWith;  //边缘节点的带宽信息
    unordered_map<string, vector<int>> site2demand;
    unordered_map<string, vector<int>> demand2site;
    int qos_constraint;
	unordered_map<string, vector<int>> usableSite; // 客户节点可以用的边缘节点；
    vector<Paramerter::Ptr> _layers; // 所有层的权重, 是一个二维数组，外层的vector是所有时刻下客户节点顺序排列
    SiteLog::Ptr log;
	vector<unordered_map<string, vector<int>>> _Weights;
    
    unordered_map<string,vector<string>> siteConnetDemand; // 边缘节点连接的客户节点
    multimap<uint64_t,string> siteConnetDemandSize; // 边缘节点连接了多少客户节点，上小下大
    unordered_map<string,int> siteUsedCnt;
    multimap<size_t,size_t> frameDmandSumMap; //存储每帧的总需求量，之后把需求量大的优先使用那些可以连上多的客户的边缘节点来分发带宽
    map<uint32_t,vector<vector<pair<string,int>>>> outputResult;

    unordered_map<string, Optim> _optimMap;
    unordered_map<string, vector<uint32_t>> fixedTime;

public:
    vector<vector<pair<string, int>>> result;
    size_t maxFree;
    vector<string> siteNode;
    vector<int> siteNodeBandwidth;
    vector<vector<int>> demand;  //时间序列下的客户节点请求

    Base(string&& _filePath);
    void solveMaxFree();
    void solveMaxAndWeight();
    void maxDeal(int& demandNow,
                vector<int>& siteNodeBand, 
                const vector<int>& demandUsableSite, 
                vector<pair<string, int>>& perResult);
    void solve();
    void save(string&& _fileName);
    ~Base() {};
    void siteNodeInit(string&& _filePath);
    void demandNodeInit(string&& _filePath);
    void qosInit(string&& _filePath);
    void qosConstraintInit(string&& _filePath);
    void paramInit();
	void findUsableSite();
    void _largestMethod(int &demandNow,
                        vector<int>& siteNodeBand,
                        const vector<int>& demandUsableSite,
                        vector<pair<string, int>>& result,
						vector<int> &siteCnt,
						const int maxFree,
						bool &flag,
                        unordered_set<int>& usedSiteIndex) const;
    void _weightMethod(int demandNow,
                        vector<int>& siteNodeBand,
                        const vector<int>& demandUsableSite,
                        vector<pair<string, int>>& result,
                        Paramerter::Ptr weight,
                        unordered_map<string, Optim::DataType>& siteFrameLog);
    void _weightMethod(int demandNow,
                    unordered_map<std::string, int>& siteBandWithCopy,
                    const vector<int>& demandUsableSite,
                    vector<pair<string, int>>& result,
                    Paramerter::Ptr weight);
    unsigned int getScore(vector<vector<pair<string, int>>>& result);
    void updateWeight(Paramerter::Ptr weight, vector<int> demandUsableSiteIndex, size_t frameId);
	void simulatedAnnealing(bool &flag, float &T);
	void updateBandwidth(int L);
	void modifySiteBandwidth(int modifiedSiteID, int perSiteModifyVal, bool flag);
	vector<int> findUsableSiteIndex(vector<int> modifiedSite, const vector<int>& demandConnectSite, uint64_t FrameID, int selfID);
	int computeSiteSumTime(const string &siteName, int FrameID);
    void averageMode(pair<string, uint64_t> demand,
                        vector<pair<string, uint64_t>> siteBWCopy,
                        vector<pair<string,uint64_t>> result);

private:
    void _weightDistribution(int demandNow,
                        const int totalDemand,
                        vector<int>& siteNodeBand,
                        const vector<int>& demandUsableSite,
                        unordered_map<string, int>& record, 
                        Paramerter::Ptr weight) const;
	bool judgeRestFree(vector<int>& _demandUsableSiteIndex ,vector<int>& _siteCnt, int _maxFree);
    void getPreFrameResult(vector<unordered_map<string,vector<pair<string, int>>>>& _preFrameResult,
                           vector<vector<pair<string, int>>>& result);
    void _updateAllWeight();
    void _frameSiteOptim(unordered_map<string, Optim::DataType>& siteFrameLog);
};

