#include "testEnv.hpp"
#include <iostream>

extern const int globDataSize;

FILE* openFile()
{
    FILE *fp;
    int filenameSize = 50;
    char filename[filenameSize];
    struct timeval tv;
    gettimeofday(&tv,NULL);

    memset(filename, 0, filenameSize);
    sprintf(filename, "results/%llu_DS_%d.csv",
            tv.tv_sec*(uint64_t)1000000+tv.tv_usec,
            globDataSize);
    fp = fopen(filename, "w");

    return fp;
}

char* determineIpc(int ipcType)
{
    if(0 == ipcType)
        return (char*) "ipcMessageQueue";
    if(1 == ipcType)
        return (char*) "ipcPipe";
    if(2 == ipcType)
        return (char*) "ipcSocket";
}

int main()
{
    std::cout << "main\n";

    int repetitionsQuantity = 100;
    const int measurementsQuantity = 12;

    int factor = 10;
    bool overloadOn = false;
    uint64_t results[measurementsQuantity ];
    memset(results, 0, sizeof(int) * measurementsQuantity );
    FILE *file = openFile();

    for(int a = 0; a<2; a++)    //overload ON/OFF
    {
        for(int ipcType=0; ipcType!=ipcWrongValue; ++ipcType)   //ipc switch
        {
            testEnv test;

            for(int i=1; i<=measurementsQuantity ; ++i)   //results number
            {

                test.setIpcType( (ipcMechanismType) ipcType );
    //            test.setIpcType(ipcMessageQueue);
        //        test.setIpcType(ipcPipe);
        //        test.setIpcType(ipcSocket);
                test.setThreadsQuantity(repetitionsQuantity);       //100 threads MAX (pipe)
                test.setMsgQuantity(i * factor);
                test.setOverloadOn(overloadOn);
                test.startTest();
                uint64_t **testResults = test.getTimeResults();


                uint64_t tmpResult = 0;

                for(int j=0; j<test.getThreadsQuantity(); ++j)
                    tmpResult += testResults[j][1] - testResults[j][0];

                tmpResult /= repetitionsQuantity;
                results[i] = tmpResult;
            }

            //print results

            char *ipcTypeStr = determineIpc(ipcType);

            printf("\nipcType %s ; overload %s ; dataSize = %d\n",
                   ipcTypeStr, overloadOn ? "ON" : "OFF", globDataSize);
            fprintf(file, "\nipcType %s ; overload %s ; dataSize = %d\n",
                    ipcTypeStr, overloadOn ? "ON" : "OFF", globDataSize);

            for(int i=1; i<=measurementsQuantity ; ++i)
            {
                printf("result %i (msgQuantity:time), %5d,%llu\n",
                       i, i*factor, results[i]);
                fprintf(file, "result %i (msgQuantity:time), %5d,%llu\n",
                        i, i*factor, results[i]);
            }

            //print results
        }

        overloadOn = ~overloadOn;
    }

    fclose(file);

	return 0;
}
