#include <iostream>
#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
string temp,line;

class base{
private:
    vector<string> demandNode;
    vector<string> siteNode;
    unordered_map<string,int> siteBandWith;  //边缘节点的带宽信息
    vector<vector<int>> demand;  //时间序列下的客户节点请求
    unordered_map<string, vector<int>> site2demand;
    int qos_constraint;
    vector<vector<pair<string, int>>> result;
public:
    base(string&& _filePath);
    void solve();
    void save();
    ~base();
    void siteNodeInit(string&& _filePath);
    void demandNodeInit(string&& _filePath);
    void qosInit(string&& _filePath);
    void qosConstraintInit(string&& _filePath);
};


base::base(string&& _filePath){ 
    // 初始化siteNodo（ ）
    siteNodeInit(_filePath + "site_bandwidth.csv");
    // 初始化demand（）
    demandNodeInit(_filePath + "demand.csv");
    // 初始化QOS()
    qosInit(_filePath + "qos.csv");
    // 初始化config（）
    qosConstraintInit(_filePath + "config.ini");
}
base::~base(){
    
}

void base::siteNodeInit(string&& _filePath){
    std::ifstream site_bandwidthfile(_filePath, std::ios::in);
    std::string line,siteNoteName,siteNodeBW;
    getline(site_bandwidthfile,temp);
    while(getline(site_bandwidthfile,line)){
        istringstream sin(line); //将整行字符串line读入到字符串流istringstream中
        getline(sin, siteNoteName, ',');
        siteNode.push_back(siteNoteName);  // 获得边缘节点名字序列
        sin >> siteNodeBW;
        pair<string,int> _frame(siteNoteName, atoi(siteNodeBW.c_str()));
        siteBandWith.insert(_frame);  // 把边缘节点和它的带宽封装成unordermap
    };
    
    cout << "siteNodeBandWith Inited " <<  siteBandWith.at("9") << endl;
}

void base::demandNodeInit(string&& _filePath){
    std::ifstream demandfile(_filePath, std::ios::in);
    getline(demandfile, line);
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
    cout << "demandNode Inited " << demandNode.size() << endl;
}

void base::qosInit(string&& _filePath){
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
    cout << "preSite2demand Inited " << site2demand.at("A")[2] << endl;
}

void base::qosConstraintInit(string&& _filePath){
    std::ifstream qos_constraintfile(_filePath, std::ios::in);
    while(getline(qos_constraintfile,temp,'='));
    qos_constraint = atoi(temp.c_str());
    cout << "qos_constraint = " <<  qos_constraint << endl;
}


int main() {
 

    base dataInit("../data/");
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



