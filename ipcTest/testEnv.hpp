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
    pthread_t overloadThread;
//    pthread_mutex_t lock;
    int *msgQueueIds;
    int **pipes;

    void printTestEnvInfo();
    void initIpc();
    void createThreads();
    void startThreads();
    void startTxThread(int i, threadArgs args);
    void startRxThread(int i, threadArgs args);
    void socketRcv(char *buf);
    void socketSnd(char *buf);

    friend void* txThread(void* args_);
    friend void *rxThread(void* args_);

public:
	testEnv();
	~testEnv();

	void createAndRunThreads();

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
