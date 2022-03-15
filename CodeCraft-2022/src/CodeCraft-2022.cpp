#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

class base{
private:
    unordered_map<string,int> siteBandWith;
    vector<string> demandName;
    vector<vector<int>> demand;
    unordered_map<string, vector<int>> qos;
    int qos_constraint;
    vector<vector<pair<string, int>>> result;

public:
    base(string&& _filePath);
    void solve();
    void save(string&& _fileName);
    ~base(){};
    void siteNodeInit(string&& _filePath);
    void demandNodeInit(string&& _filePath);
    void qosInit(string&& _filePath);
    void qosConstraintInit(string&& _filePath);
};


base::base(string&& _filePath){ 
    // 初始化siteNodo（ ）
    siteNodeInit(_filePath + "site_bandwidth.csv");
    // 初始化demand（）
    // 初始化QOS()

    // 初始化config（）

}

void base::qosConstraintInit(string&& _filePath){
    
}

void siteNodeInit(string& _filePath);
void creatSiteNodeData(string& _filePath, unordered_map<string,int>& _site_bandwidth);
int getSiteNodeBandWith(string& _name, unordered_map<string,int>& _site_bandwidth);


int main() {
    // 边缘节点信息
    cout << "output_test" << endl;
    string site_bandwidth_filePath = "../data/site_bandwidth.csv";
    string demand_filePath = "../data/demand.csv";
    unordered_map<string,int> site_bandwidth;
    creatSiteNodeData(site_bandwidth_filePath, site_bandwidth);

    string line;
    std::ifstream _data(demand_filePath, std::ios::in);
    while (std::getline(_data,line))
    {
        istringstream sin(line); //将整行字符串line读入到字符串流istringstream中
        while (getline(sin,))
        {
            /* code */
        }
        
    }
    
    cout << site_bandwidth.at("AJ") << endl;
    // istringstream sin(line); //将整行字符串line读入到字符串流istringstream中
    // getline(sin, siteName, ',');
    // sin >> bandwith;
    // pair<string,int> Frame (siteName, bandwith);
    // site_bandwidth.insert(Frame);
    // /* code */
    // cout << site_bandwidth.size() << endl;
	return 0;
}


void creatSiteNodeData(string& _filePath, unordered_map<string,int>& _site_bandwidth){
    _filePath+="s";
    string line;
    string siteName,sbandWidth;
    std::ifstream _data(_filePath, std::ios::in);
    std::getline(_data,line);
    if(!_data.is_open())
    {
        std::cout << "Error: opening file fail" << std::endl;
        std::exit(1);
    }
    while (std::getline(_data,line))
    {
        std::istringstream sin(line);
        getline(sin, siteName, ',');
        getline(sin, sbandWidth, ',');
        int bandWidth = atoi(sbandWidth.c_str());
        pair<string,int> temp(siteName,bandWidth);
        _site_bandwidth.insert(temp);
    }
}

int getSiteNodeBandWith(string& _name, unordered_map<string,int>& _site_bandwidth){
    return _site_bandwidth.at(_name);
}

void siteNodeInit(string&& _filePath){
    
}

void base::save(string&& _fileName) {
    ofstream file(_fileName, ios_base::out);
    int customerNum = demandName.size();
    for(size_t i = 0; i < result.size(); ++i) {
        int curCustomerID = i % customerNum; 
        // 客户ID
        file << "C" << demandName[curCustomerID] << ":";
        if (result[i].empty()) {
            file << "\n";
            continue;
        } 
        // 分配方案
        for (size_t j = 0; j < result[i].size(); ++j) {
            file << "<" << result[i][j].first << "," 
                << result[i][j].second << ">";
            if (j == result[i].size() - 1) {
                file << "\n";
            } else {
                file << ",";
            }
        }
    }
    file.close();
} 