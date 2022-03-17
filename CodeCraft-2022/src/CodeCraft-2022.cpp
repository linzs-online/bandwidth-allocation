#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
string temp,line;

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
public:
    Base(string&& _filePath);
    void solve();
    void save(string&& _fileName);
    ~Base() {};
    void siteNodeInit(string&& _filePath);
    void demandNodeInit(string&& _filePath);
    void qosInit(string&& _filePath);
    void qosConstraintInit(string&& _filePath);
	void findUsableSite();
    void _largestMethod(int demandNow,
                        vector<int>& siteNodeBand, 
                        const vector<int>& demandUsableSite, 
                        vector<pair<string, int>>& result) const;
    void _avergeMethod(int demandNow,
                        vector<int>& siteNodeBand, 
                        const vector<int>& demandUsableSite, 
                        vector<pair<string, int>>& result) const;
    void _avergeDistribution(int demandNow,
                        vector<int>& siteNodeBand, 
                        const vector<int>& demandUsableSite, 
                        unordered_map<string, int>& record) const;
};


Base::Base(string&& _filePath){ 
    // 初始化siteNodo（ ）
    siteNodeInit(_filePath + "site_bandwidth.csv");
    // 初始化demand（）
    demandNodeInit(_filePath + "demand.csv");
    // 初始化QOS()
    qosInit(_filePath + "qos.csv");
    // 初始化config（）
    qosConstraintInit(_filePath + "config.ini");
	//找到满足qos的site
	findUsableSite();
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
    };
    
    //cout << "siteNodeBandWith Inited " <<  siteBandWith.at("9") << endl;
}

void Base::demandNodeInit(string&& _filePath){
    std::ifstream demandfile(_filePath, std::ios::in);
    getline(demandfile, line,'\r');
	getline(demandfile, temp,'\n');
    istringstream sin(line);
    getline(sin, temp, ',');
    while(getline(sin, temp, ','))
    {
        demandNode.emplace_back(temp); // 获得客户节点的名字序列
    }
    vector<int> demandFrame;
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

int main() {
    //Base dataInit("/data/");
    Base dataInit("/data/");
	dataInit.solve();
    //dataInit.save("/output/solution.txt");
	dataInit.save("/output/solution.txt");
	return 0;
}



void Base::save(string&& _fileName) {
    ofstream file(_fileName, ios_base::out);
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

void Base::solve() {
    for(auto demandNow : demand){
        vector<int> siteNodeBandwidthCopy = siteNodeBandwidth; // 边缘节点带宽量的拷贝，每一轮拷贝一次
        for(size_t i = 0; i < demandNow.size(); ++i){ // 遍历客户节点
            vector<pair<string, int>> resultCustom; // 每一轮的结果
            vector<int> demandUsableSite = usableSite[demandNode[i]]; // 获取当前时刻可用的边缘节点
            // _largestMethod(demandNow[i], siteNodeBandwidthCopy, demandUsableSite, resultCustom);
            _avergeMethod(demandNow[i], siteNodeBandwidthCopy, demandUsableSite, resultCustom);
            result.push_back(resultCustom);
        }
    }
}


/**
 * @brief 最大划分
 * @param demanNow 某个客户需求量
 * @param siteNodeBand 边缘节点带宽量
 * @param demandUsableSite 延迟满足的边缘节点
 * @param result 结果
 */
void Base::_largestMethod(int demandNow,
            vector<int>& siteNodeBand, 
            const vector<int>& demandUsableSite, 
            vector<pair<string, int>>& result) const {
    for (size_t j = 0; j < demandUsableSite.size(); ++j) { // 遍历可用边缘节点
        if (siteNodeBand[demandUsableSite[j]] == 0) {
            continue;
        }
        if (demandNow <= siteNodeBand[demandUsableSite[j]]) {                                                               // 若边缘节点可以分配足够带宽
            siteNodeBand[demandUsableSite[j]] -= demandNow; // 减去分配带宽
            pair<string, int> y(siteNode[demandUsableSite[j]], demandNow);
            result.push_back(y);
            break;
        }
        else {                                                               // 边缘节点带宽量不够
            demandNow -= siteNodeBand[demandUsableSite[j]]; // 将剩余的带宽量分配
            pair<string, int> x(siteNode[demandUsableSite[j]], siteNodeBand[demandUsableSite[j]]);
            siteNodeBand[demandUsableSite[j]] = 0;
            result.push_back(x);
        }
    }
}


/**
 * @brief 均分策略进行分配
 * @param demanNow 某个客户需求量
 */
void Base::_avergeMethod(int demandNow,
            vector<int>& siteNodeBand, 
            const vector<int>& demandUsableSite, 
            vector<pair<string, int>>& result) const {
    unordered_map<string, int> record;
    _avergeDistribution(demandNow, siteNodeBand, 
                demandUsableSite, record);
    auto it = record.cbegin();
    while (it != record.cend()) {
        pair<string, int> tmp(it->first, it->second);
        result.emplace_back(tmp);
        it++;
    }
}


/**
 * @breif 查询带宽
*/
inline int bandInit(const string& key, const 
            unordered_map<string, int>& record) {
    if (record.count(key)) {
        return record.at(key);
    }
    return 0;
}



void Base::_avergeDistribution(int demandNow,
                vector<int>& siteNodeBand, 
                const vector<int>& demandUsableSite, 
                unordered_map<string, int>& record) const {
    // 均分策略
    for (size_t i = 0; i < demandUsableSite.size(); ++i) {
        size_t useableNumber = demandUsableSite.size() - i;
        if (demandNow == 0) {
            return;
        }
        int site = demandUsableSite[i];
        if (siteNodeBand[site] == 0) {
            continue;
        }
        int averDemand = demandNow / useableNumber;
        int res = demandNow - averDemand * useableNumber;
        // 获得边缘节点分配量
        int band = bandInit(siteNode[site], record);
        auto getBand = [&](int demand) {
            if (demand == 0) {
                return;
            }
            if (siteNodeBand[site] >= demand) {
                band += demand;
                siteNodeBand[site] -= demand;
            } else {
                band += siteNodeBand[site];
                useableNumber -= 1;
                siteNodeBand[site] = 0;
            }
            
        };
        getBand(res);
        getBand(averDemand);
        // 更新
        demandNow -= band;
        record[siteNode[site]] = band;
    }

    // 递归分配余量
    _avergeDistribution(demandNow, siteNodeBand, demandUsableSite, record);
}

