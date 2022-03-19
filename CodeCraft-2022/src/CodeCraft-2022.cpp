#include "../inc/base.h"
#include "../inc/tic_toc.h"



int main() {
    using std::cout;
    using std::endl;
    TicToc clockCaculate;
    //test("../../data/demand.csv");
    //Base dataInit("/Users/dengyinglong/bandwidth-allocation/data/");
    Base dataInit("../data/");
	dataInit.solve();
	//dataInit.save("solution.txt");
	dataInit.save("solution.txt");

    cout << dataInit.getScore() << std::endl;
    
    cout << "所有程序运行时间: " << clockCaculate.toc()  << "ms" << std::endl;
	return 0;
}
