#ifndef _Data_Base_H_
#define _Data_Base_H_

#include <iostream>
#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
void creatSiteNodeData(string& _filePath, unordered_map<string,int>& _site_bandwidth);
int getSiteNodeBandWith(string& _name, unordered_map<string,int>& _site_bandwidth);

#endif