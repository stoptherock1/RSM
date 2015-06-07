#include "testEnv.hpp"
#include <iostream>


int main()
{
	std::cout << "main\n";

//    for(int i=0; i<10; ++i)
    {
        testEnv test;
        test.setIpcType(ipcMessageQueue);
//        test.setIpcType(ipcPipe);
//        test.setIpcType(ipcSocket);
        test.setThreadsQuantity(10);       //100 threads MAX (pipe)
        test.setMsgQuantity(1);
        test.setOverloadOn(false);
        test.startTest();
        uint64_t **testResults = test.getTimeResults();

        for(int i=0; i<test.getThreadsQuantity(); ++i)
            std::cout << testResults[i][1] - testResults[i][0] << std::endl;
    }



	return 0;
}
