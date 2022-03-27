#include "../inc/base.h"
#include "../inc/tic_toc.h"

int main() {
    using std::cout;
    using std::endl;
//    Base dataInit("/Users/dengyinglong/bandwidth-allocation/data/");
    Base dataInit("/data/");
	// dataInit.solveFree();
	dataInit.solve();
    if (!dataInit.bestResult.empty()) {
        cout << "保存结果\n";
        dataInit.save2("/output/solution.txt");
        //dataInit.save2("/output/solution.txt");
        //cout << dataInit.getScore(dataInit.result) << endl;
    }
    //cout << "所有程序运行时间: " << clockCaculate.toc()  << "ms" << std::endl;
	return 0;
}