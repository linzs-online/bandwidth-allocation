#include <dataBase.hpp>

void creatSiteNodeData(string& _filePath, unordered_map<string,int>& _site_bandwidth){
    string line;
    string siteName,sbandWidth;
    std::ifstream data(_filePath, std::ios::in);
    std::getline(data,line);
    if(!data.is_open())
    {
        std::cout << "Error: opening file fail" << std::endl;
        std::exit(1);
    }
    while (std::getline(data,line))
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