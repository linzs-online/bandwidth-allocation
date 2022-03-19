#include <random>
#include <unordered_map>
#include <unordered_set>
#include "../inc/paramter.h"


/**
 * @brief 并非标准的softmax,并未使用指数，算比例用的
 * @param start 
 * @param offValue 增量更新值
 */
void Paramerter::softmax(size_t start, float offValue) {
        float sum = std::accumulate(value.begin() + start, value.end(), 0.0f);
        for (size_t i = start; i < value.size(); i++) {
            if (offValue != 0.0f) {
                float tmp = value[i] + (value[i] / sum) * offValue;
                if (tmp < 0) {
                    offValue += tmp;
                    value[i] = 0.0f;

                } else {
                    value[i] = tmp;
                }
            } else {
                value[i] = value[i] / sum;
            }
        }
}


void Paramerter::normal(double mean, double stddev) {
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> rng{mean, stddev};
    for (size_t i = 0; i < value.size(); i++) {
        value[i] = std::abs(rng(gen));
    }
}

//unordered_map<string,pair<int,vector<double>>> siteLogMap;
SiteLog::SiteLog(vector<string> _siteNodeName, vector<int> _siteNodeBandWith, int _frameSize):siteNodeName(_siteNodeName){
    //边缘节点的带宽，用于计算每个边缘节点的利用率
    for(auto i = 0; i < _siteNodeBandWith.size(); i++){
        pair<string,int> _temp(_siteNodeName[i], _siteNodeBandWith[i]);
        sitBandWithMap.insert(_temp);
    }
    vector<double> perSiteNodeUsage(_frameSize,0);
    for(auto& nodeName: _siteNodeName){
        pair<int, vector<double>> _pair(0,perSiteNodeUsage);
        pair<string,pair<int,vector<double>>> _history(nodeName,_pair);
        siteLogMap.insert(_history);
    }
}
//unordered_map<string,pair<int,vector<double>>> siteLogMap;
void SiteLog::write2Log(vector<vector<pair<string, int>>>::iterator itFirst,
                        vector<vector<pair<string, int>>>::iterator itEnd, int frameID){
    //记录每个边缘节点在这一帧中的带宽分配总量
    unordered_map<string, int> nowFrameSiteUsed;
    //记录这一帧使用过的边缘节点的名字
    unordered_set<string> nowFrameSiteName;
    for(auto& name : siteNodeName){
        pair<string,int> _temp(name,0);
        nowFrameSiteUsed.insert(_temp);
    }
    for(auto it = itEnd; it != itFirst; it--){
        for(auto perDresut = it->begin(); perDresut != it->end(); perDresut++){
            nowFrameSiteName.insert(perDresut->first);
            nowFrameSiteUsed[perDresut->first] += perDresut->second;
        }
    }
    for(auto siteName : nowFrameSiteName){
        siteLogMap.at(siteName).first++;
        double x = nowFrameSiteUsed.at(siteName); //隐式转换
        double y = sitBandWithMap.at(siteName);
        siteLogMap.at(siteName).second[frameID] = x/y; //返回使用率
    }
}