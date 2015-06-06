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
#include <dispatch/dispatch.h>
#include <iomanip>

class testEnv;

struct msgData
{
    long type;
    char *data;
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
    int msgQueueId;
    int msgQuantity;
    ipcMechanismType ipcType;
    testEnv* thisPtr;
};

class testEnv
{
private:
    int threadsQuantity;       //number of pairs (tx,rx)
    int msgSize;
    int msgQuantity;
    bool overloadOn;
    ipcMechanismType ipcType;

    pthread_t *txThreads;
    pthread_t *rxThreads;
    dispatch_semaphore_t *semaphores;
    pthread_t overloadThread;
    int *msgQueueIds;
    int **pipes;
    threadArgs *args;

    void printTestEnvInfo();
    void initIpc();
    void initThreads();
    void createAndStartThreads();
    void rxSocket(char *buf, testEnv *thisPtr);
    void txSocket(char *buf, testEnv* thisPtr);

    friend void* txThread(void* args_);
    friend void *rxThread(void* args_);

public:
	testEnv();
	~testEnv();

    void startTest();

    int getThreadsQuantity() const;
    void setThreadsQuantity(int value);
    int getMsgSize() const;
    void setMsgSize(int value);
    int getMsgQuantity() const;
    void setMsgQuantity(int value);
    bool getOverloadOn() const;
    void setOverloadOn(bool value);
    ipcMechanismType getIpcType() const;
    void setIpcType(const ipcMechanismType &value);
};

#endif /*TESTENV_H*/
