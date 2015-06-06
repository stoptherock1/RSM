#include "testEnv.hpp"
#include <iostream>

int main()
{
	std::cout << "main\n";

    for(int i=0; i<10; ++i)
    {
        testEnv test;
//        test.setIpcType(ipcSocket);
        test.setIpcType(ipcPipe);
        test.setThreadsQuantity(100);       //100 threads MAX (pipe)
        test.setMsgQuantity(1);
        test.setMsgSize(100);
        test.setOverloadOn(false);
        test.startTest();
//        sleep(1);
    }

	return 0;
}
