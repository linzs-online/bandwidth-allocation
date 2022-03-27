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
 

    unordered_map<string, Optim> _optimMap;
    unordered_map<string, vector<uint32_t>> fixedTime;
    unordered_map<uint32_t, unordered_map<string, uint64_t>> frameMap;
    unordered_map<uint32_t, unordered_map<string,int>> sitePerFrameBW;
    unordered_map<string, multimap<uint64_t, uint32_t>> sortMap; 
    

public:
    vector<vector<pair<string, int>>> result;
    size_t maxFree;
    vector<string> siteNode;
    vector<int> siteNodeBandwidth;
    vector<vector<int>> demand;  //时间序列下的客户节点请求
    unordered_map<uint32_t,unordered_map<string,unordered_map<string, pair<string,int>>>> bestResult;
    //unordered_map<uint32_t,unordered_map<string,unordered_map<string, pair<string,int>>>> goodResult;
    uint32_t bestScore;

    Base(string&& _filePath);
    void solveFree();
    void solveFree2();
    void solve();
    void save(string&& _fileName);
    void save2(string&& _fileName);
    ~Base() {};
    void siteNodeInit(string&& _filePath);
    void demandNodeInit(string&& _filePath);
    void qosInit(string&& _filePath);
    void qosConstraintInit(string&& _filePath);
    void paramInit();
	void findUsableSite();
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
    uint32_t getScore();
	void updateBandwidth(int L);
	void modifySiteBandwidth(int modifiedSiteID, int perSiteModifyVal, bool flag);
	vector<int> findUsableSiteIndex(vector<int> modifiedSite, const vector<int>& demandConnectSite, uint64_t FrameID, int selfID);
	int computeSiteSumTime(const string &siteName, int FrameID);
    void initalzeResult(uint16_t&& size);

private:
    void _weightDistribution(int demandNow,
                        const int totalDemand,
                        vector<int>& siteNodeBand,
                        const vector<int>& demandUsableSite,
                        unordered_map<string, int>& record, 
                        Paramerter::Ptr weight) const;
    void getPreFrameResult(vector<unordered_map<string,vector<pair<string, int>>>>& _preFrameResult,
                           vector<vector<pair<string, int>>>& result);
    void _frameSiteOptim(unordered_map<string, Optim::DataType>& siteFrameLog);
};

