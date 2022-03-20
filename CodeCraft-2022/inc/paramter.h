#pragma once

#include <memory>
#include <vector>
#include <numeric>

using namespace std;
class Paramerter {
public:
    using Ptr = std::shared_ptr<Paramerter>;
    std::vector<float> value;

    Paramerter() {};
    void softmax(size_t start = 0, float offValue = 0);
    void normal(double mean = 0, double stddev = 1);
    void init(size_t threshold);
};


class SiteLog{
public:
    using Ptr = std::shared_ptr<SiteLog>;
    unordered_map<string,pair<int,vector<double>>> siteLogMap;
    unordered_map<string,int> sitBandWithMap;
    vector<string> siteNodeName;
    //记录每个边缘节点每帧分配结果中的带宽分配总量
    unordered_map<string, int> nowFrameSiteUsed;
    //记录每帧中使用过的边缘节点的名字
    unordered_set<string> nowFrameSiteName;
    size_t oldFrameId = 0;

    SiteLog(vector<string> _siteNodeName, vector<int> _siteNodeBandWith, int _frameSize);
    void write2Log(vector<pair<string, int>> _Result, size_t frameID);
    void logClear();
};
