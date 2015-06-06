#include "testEnv.hpp"
#include <iostream>

int main()
{
	std::cout << "main\n";

    for(int i=0; i<1; ++i)
    {
        testEnv test;
        test.setIpcType(ipcSocket);
        test.setThreadsQuantity(2);       //100 threads MAX
        test.setMsgQuantity(1);
        test.setMsgSize(100);
        test.setOverloadOn(false);
        test.startTest();
    }

	return 0;
}
