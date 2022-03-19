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
};


class SiteLog{
public:
    unordered_map<string,pair<int,vector<double>>> siteLogMap;
    unordered_map<string,int> sitBandWithMap;
    vector<string> siteNodeName;

    SiteLog(vector<string> _siteNodeName, vector<int> _siteNodeBandWith, int _frameSize);
    void write2Log(vector<vector<pair<string, int>>>::iterator itFirst,
                    vector<vector<pair<string, int>>>::iterator itEnd, int frameID);
};
