#include "testEnv.hpp"
#include <iostream>

int main()
{
	std::cout << "main\n";

    testEnv test;
    test.setIpcType(ipcSocket);
    test.setMsgQuantity(5);
    test.setMsgSize(100);
    test.setOverloadOn(false);
    test.setThreadsQuantity(1);

	test.createAndRunThreads();

	return 0;
}
