#pragma once

#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include "paramter.h"


using std::vector;
using std::string;
using std::unordered_map;
using std::pair;

class Base{
private:
    vector<string> demandNode;
    vector<string> siteNode;
    vector<int> siteNodeBandwidth;
    unordered_map<string,int> siteBandWith;  //边缘节点的带宽信息
    vector<vector<int>> demand;  //时间序列下的客户节点请求
    unordered_map<string, vector<int>> site2demand;
    unordered_map<string, vector<int>> demand2site;
    int qos_constraint;
    vector<vector<pair<string, int>>> result;
	unordered_map<string, vector<int>> usableSite; // 客户节点可以用的边缘节点；
    
    vector<Paramerter::Ptr> _layers;

public:
    Base(string&& _filePath);
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
                        Paramerter::Ptr weight) const;
    unsigned int getScore();

private:
    void _weightDistribution(int demandNow,
                        const int totalDemand,
                        vector<int>& siteNodeBand,
                        const vector<int>& demandUsableSite,
                        unordered_map<string, int>& record, 
                        Paramerter::Ptr weight) const;
	bool judgeRestFree(vector<int> siteCnt, int maxFree);
    vector<unordered_map<string,vector<pair<string, int>>>> getPreFrameResult();

};


void test(string&& _filePath);