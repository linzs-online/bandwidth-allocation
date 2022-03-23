#pragma once

#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "paramter.h"



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
    unordered_map<string, Optim> _optimMap;

public:
    vector<vector<pair<string, int>>> result;
    size_t maxFree;
    vector<string> siteNode;
    vector<int> siteNodeBandwidth;
    vector<vector<int>> demand;  //时间序列下的客户节点请求

    Base(string&& _filePath);
    void solveMaxFree();
    void solveMaxAndWeight();
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
                        Paramerter::Ptr weight);
    unsigned int getScore(vector<vector<pair<string, int>>>& result);
    void updateWeight(Paramerter::Ptr weight, vector<int> demandUsableSiteIndex, size_t frameId);
	void simulatedAnnealing(bool &flag, float &T);

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

	void solveWeight();

	void _bandAllocate(const string &name);

	void solveAnne();
};

