#ifndef TESTENV_H
#define TESTENV_H

#include <pthread.h>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <iomanip>
#include <sys/syscall.h>
#include <vector>
#include <sys/time.h>

const int globDataSize = 100;
class testEnv;

struct msgData
{
    long type;
    char data[globDataSize];
};

enum ipcMechanismType
{
    ipcMessageQueue = 0,
    ipcPipe,
    ipcSocket,
    ipcWrongValue
};

struct threadArgs
{
    int threadNumber;
    testEnv* thisPtr;
};

class testEnv
{
public:
    int threadsQuantity;       //number of pairs (tx,rx)
    int dataSize;
    int msgQuantity;
    bool overloadOn;
    ipcMechanismType ipcType;

    pthread_t *txThreads;
    pthread_t *rxThreads;
    sem_t *semaphores;
    pthread_t overloadThread;
    int *msgQueueIds;
    int **pipes;
    int *rxPorts;
    int *txPorts;
    threadArgs *args;
    std::vector<void*> toFree;



    void initIpc();
    void initThreads();
    void createAndStartThreads();

    friend void* txThread(void* args_);
    friend void *rxThread(void* args_);

    friend inline void rxTimestamp(threadArgs &args, int &id);
    friend inline void txTimestamp(threadArgs &args, int &id);

public:
    uint64_t **timeResults;

	testEnv();
	~testEnv();

    void startTest();

    void printTestEnvInfo();
    int getThreadsQuantity() const;
    void setThreadsQuantity(int value);
    int getDataSize() const;
    void setDataSize(int value);
    int getMsgQuantity() const;
    void setMsgQuantity(int value);
    bool getOverloadOn() const;
    void setOverloadOn(bool value);
    ipcMechanismType getIpcType() const;
    void setIpcType(const ipcMechanismType &value);
    int *getMsgQueueIds() const;
    void setMsgQueueIds(int *value);
    int **getPipes() const;
    void setPipes(int **value);
    uint64_t **getTimeResults() const;
    void setTimeResults(uint64_t **value);
};

#endif /*TESTENV_H*/
