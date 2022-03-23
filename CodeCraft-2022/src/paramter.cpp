#include <random>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cassert>
#include "../inc/paramter.h"


/**
 * @brief 并非标准的softmax,并未使用指数，算比例用的
 * @param start 
 * @param offValue 增量更新值
 */
void Paramerter::softmax(size_t start, float offValue) {
        float sum = std::accumulate(value.begin() + start, value.end(), 0.0f);
        if(sum == 0) {
            init(value.size());
            return;
        }
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
	auto time = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 gen(time);
    std::normal_distribution<> rng{mean, stddev};
    for (size_t i = 0; i < value.size(); i++) {
        value[i] = std::abs(rng(gen));
    }
}

void Paramerter::init(size_t threshold) {
    auto time = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(time);
    size_t id = gen() % threshold;
    value[id] = 1;
}


//unordered_map<string,pair<int,vector<double>>> siteLogMap;
SiteLog::SiteLog(vector<string> _siteNodeName, vector<int> _siteNodeBandWith, int _frameSize):siteNodeName(_siteNodeName){
    //边缘节点的带宽，用于计算每个边缘节点的利用率
    int temp = _siteNodeBandWith.size();
    for(int i = 0; i < temp; i++){
        pair<string,int> _temp(_siteNodeName[i], _siteNodeBandWith[i]);
        sitBandWithMap.insert(_temp);
    }
    vector<double> perSiteNodeUsage(_frameSize,0);
    for(auto& nodeName: _siteNodeName){
        pair<int, vector<double>> _pair(0,perSiteNodeUsage);
        pair<string,pair<int,vector<double>>> _history(nodeName,_pair);
        siteLogMap.insert(_history);
    }
    for(auto& name : siteNodeName){
        pair<string,int> _temp(name,0);
        nowFrameSiteUsed.insert(_temp);
    }
}
void SiteLog::write2Log(vector<pair<string, int>> _Result, size_t frameID){
    //说明当前还在处理同一帧的分配结果
    if(oldFrameId == frameID){
        for(auto _pair : _Result){
            nowFrameSiteName.insert(_pair.first);
            nowFrameSiteUsed.at(_pair.first) += _pair.second;
        }
    }
    else{ //已经是新的一帧了,先把上一帧的数据进行一次统计
        for(auto siteName : nowFrameSiteName){
            siteLogMap.at(siteName).first++; //使用次数
            double x = nowFrameSiteUsed.at(siteName); //隐式转换
            double y = sitBandWithMap.at(siteName);
            siteLogMap.at(siteName).second[oldFrameId] = x/y; //返回使用率
        }
        oldFrameId = frameID;
        //清理上一帧的缓存
        nowFrameSiteName.clear();
        for(auto it = nowFrameSiteUsed.begin(); it != nowFrameSiteUsed.end(); it++){
            it->second = 0;
        }
        //处理当前帧进来的第一个客户请求分配方案
        for(auto _pair : _Result){
            nowFrameSiteName.insert(_pair.first);
            nowFrameSiteUsed.at(_pair.first) += _pair.second;
        }
    }
}

void SiteLog::logClear(){
    for(auto siteName : siteNodeName){
        siteLogMap.at(siteName).first = 0;
        fill(siteLogMap.at(siteName).second.begin(),siteLogMap.at(siteName).second.end(),0);
    }
}


int Optim::mean() {
    int64_t avr = 0;
    for(size_t i = 0; i < value.size(); ++i) {
        avr += (value[i].first - avr) / (i + 1);
    }
    return avr;
}


uint32_t Optim::mid() {
    size_t n = value.size();
    auto mid = value.begin() + n / 2;
    std::nth_element(value.begin(), mid, value.end(),
            [](Optim::DataType a, Optim::DataType b) { return a.first < b.first;});
    return mid->first;
}


void Optim::step(int _mean) {
    for (size_t i = 0; i < value.size(); ++i) {
        value[i].second += float(_mean - value[i].first) / 1000;
        if (value[i].second < 0.0f) {
            value[i].second = 0.0f;
        }
    }
}
