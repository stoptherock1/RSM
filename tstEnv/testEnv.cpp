#include "testEnv.hpp"

pthread_mutex_t stdOutLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timeLock = PTHREAD_MUTEX_INITIALIZER;

#define RX_PORT 18000
#define TX_PORT 28000
//#define DEBUG_PRINTS
//#define DEEP_DEBUG_PRINTS


#define info(arg)   { \
                        if(0 != pthread_mutex_lock(&stdOutLock) ) \
                            std::cout << "\e[0;31m[ ERROR WHILE LOCKING STDOUTMUTEX]\e[0m"; \
                        std::cout << "[ INFO ]  : " \
                                  << "txThread (line:  124) : " \
                                  << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                  << arg << std::endl; \
                        if(0 != pthread_mutex_unlock(&stdOutLock) ) \
                            std::cout << "\e[0;31m[ ERROR WHILE UNLOCKING STDOUTMUTEX]\e[0m"; \
                    }

#define error(arg)  { \
                        if(0 != pthread_mutex_lock(&stdOutLock) ) \
                            std::cout << "\e[0;31m[ ERROR WHILE LOCKING STDOUTMUTEX]\e[0m"; \
                        std::cout << "\e[0;31m[ ERROR ]\e[0m : " \
                                  << __FUNCTION__\
                                  << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                  << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                  << arg \
                                  << std::endl; \
                        if(0 != pthread_mutex_unlock(&stdOutLock) ) \
                            std::cout << "\e[0;31m[ ERROR WHILE UNLOCKING STDOUTMUTEX]\e[0m"; \
                    }



#ifdef DEBUG_PRINTS
    #define debug(arg)  { \
                            if(0 != pthread_mutex_lock(&stdOutLock) ) \
                                std::cout << "\e[0;31m[ ERROR WHILE LOCKING STDOUTMUTEX]\e[0m"; \
                            std::cout << "\e[0;36m[ DEBUG ] : " \
                                      << __FUNCTION__ \
                                      << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                      << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                      << arg \
                                      << "\e[0m" << std::endl; \
                            if(0 != pthread_mutex_unlock(&stdOutLock) ) \
                                std::cout << "\e[0;31m[ ERROR WHILE UNLOCKING STDOUTMUTEX]\e[0m"; \
                        }
#else
    #define debug(arg)
#endif

#ifdef DEEP_DEBUG_PRINTS
    #define debug2(arg)  { \
                            if(0 != pthread_mutex_lock(&stdOutLock) ) \
                                std::cout << "\e[0;31m[ ERROR WHILE LOCKING STDOUTMUTEX]\e[0m"; \
                            std::cout << "\e[0;36m[ DEBUG ] : " \
                                      << __FUNCTION__ \
                                      << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                      << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                      << arg \
                                      << "\e[0m" << std::endl; \
                            if(0 != pthread_mutex_unlock(&stdOutLock) ) \
                                std::cout << "\e[0;31m[ ERROR WHILE UNLOCKING STDOUTMUTEX]\e[0m"; \
                        }
#else
    #define debug2(arg)
#endif


__inline__ pid_t tid()
{
    return (pid_t) syscall (SYS_gettid);
}

inline void firstTimestamp(threadArgs &args, int &id)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(0 != pthread_mutex_lock(&timeLock) )
        error("An error occured while locking mutex; errno = "
                  << errno << ": " << strerror(errno) );

    args.thisPtr->timeResults[id][0] = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;

    if(0 != pthread_mutex_unlock(&timeLock) )
        error("An error occured while locking mutex; errno = "
                  << errno << ": " << strerror(errno) );
}

inline void secondTimestamp(threadArgs &args, int &id)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(0 != pthread_mutex_lock(&timeLock) )
        error("An error occured while locking mutex; errno = "
                  << errno << ": " << strerror(errno) );

    args.thisPtr->timeResults[id][1] = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;

    if(0 != pthread_mutex_unlock(&timeLock) )
        error("An error occured while locking mutex; errno = "
                  << errno << ": " << strerror(errno) );
}

void *txThread(void* args_)
{
    debug2("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;
    int ret =  0;
    int &msgQueueId = args.thisPtr->msgQueueIds[id];
    int connectionSocket = -1;
    struct sockaddr_in rxAddr;
    struct sockaddr_in txAddr;



    // init IPC  ////////////
    switch(args.thisPtr->ipcType)
    {
        case ipcMessageQueue:
        {
            //create msg queue
            key_t key = ftok("main.cpp", id);
            if(-1 == key)
                error("An error occured while performing ftok; errno = "
                          << errno << ": " << strerror(errno) );

            msgQueueId = msgget(key, 0644 | IPC_CREAT);
            if(-1 == msgQueueId)
                error("An error occured while creating new message queueu; errno = "
                          << errno << ": " << strerror(errno) );

            break;
        }

        case ipcPipe:
        {
            //create pipe
            args.thisPtr->pipes[id] = new int[2];
            ret = pipe( args.thisPtr->pipes[id] );
            if(0 != ret)
                error("An error occured while creating a pipe; errno = "
                          << errno << ": " << strerror(errno) );
            break;
        }

        case ipcSocket:
        {
            memset(&rxAddr, 0, sizeof(struct sockaddr_in) );

            connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);
            if (connectionSocket < 0)
               error("An error occured while opening socket");

            txAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            txAddr.sin_family      = AF_INET;
            txAddr.sin_port        = htons( args.thisPtr->txPorts[id] );
            debug2("TX_PORT: " << args.thisPtr->txPorts[id] );

            rxAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            rxAddr.sin_family      = AF_INET;
            rxAddr.sin_port        = htons( args.thisPtr->rxPorts[id] );
            debug2("RX_PORT: " <<  args.thisPtr->rxPorts[id] );

            ret = bind( connectionSocket, (struct sockaddr *) &txAddr, sizeof(txAddr) );
            if (0 != ret)
                error("An error occured while performing bind; errno = "
                          << errno << ": " << strerror(errno) );

            debug2("before wait");
            ret = sem_wait( &args.thisPtr->semaphores[id] );

            int value;
            sem_getvalue(&args.thisPtr->semaphores[id], &value);
            if (0 != ret)
            {

                error("An error occured while performing sem_wait; errno = "
                          << errno << ": " << strerror(errno)
                      << " The value of the " << id <<  " semaphor is " << value);
            }

            debug2("after wait");
            break;
        }

        default:
            break;
    }


    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {
        //prepare data to send
        int dataSize = args.thisPtr->dataSize;
        char buf[dataSize];

        memset(buf, 0, dataSize);
        snprintf(buf, dataSize, "tx %d MESSAGE %d", id, i);
        buf[dataSize - 1] = '\0';

        info("Message \"" << buf << "\" was created by [TX " << id << "]")

        switch(args.thisPtr->ipcType)
        {
        case ipcMessageQueue:
        {
            msgData msg;
            long msgType = 1;

            //prepare message
            msg.type = msgType;
            memset(msg.data, 0, dataSize);
            memcpy(msg.data, buf, dataSize);

            //send message
            ret = msgsnd(msgQueueId, &msg, dataSize, 1);
            if(-1 == ret)
            error("An error occured while performing msgrcv; errno = "
                      << errno << ": " << strerror(errno) );

            ret = sem_post( &args.thisPtr->semaphores[id] );
             if(0 != ret)
                 error("An error occured while waking a thread; errno = "
                       << errno << ": " << strerror(errno) );

            break;
        }

        case ipcPipe:
        {
            ret = write(args.thisPtr->pipes[id][1], buf, dataSize);
            if(-1 == ret)
                error("An error occured while writing to pipe; errno = "
                          << errno << ": " << strerror(errno) );

            debug("Message \"" << buf << "\" was sent  {PIPE}");

            ret = sem_post( &args.thisPtr->semaphores[id] );
            if(0 != ret)
                error("An error occured while waking a thread; errno = "
                      << errno << ": " << strerror(errno) );

            break;
        }

        case ipcSocket:
        {
            ret = connect( connectionSocket,
                           (struct sockaddr *)&rxAddr,
                           sizeof(rxAddr) );
            if (0 != ret)
                error("An error occured while performing connect; errno = "
                          << errno << ": " << strerror(errno) );

            ret = send(connectionSocket, buf, strlen(buf), 0);
            if (-1 == ret)
                error("An error occured while performing send; errno = "
                          << errno << " : " << strerror(errno) );
            debug("Message \"" << buf << "\" was sent  {SOCKET}");

            break;
        }

        default:
        {
            break;
        }
        }


        info("Message \"" << buf << "\" was sent by [TX " << id << "]");
    }

    firstTimestamp(args, id);


    //cleanup
    if(ipcSocket == args.thisPtr->ipcType)
    {
        ret = close(connectionSocket);
        if (0 != ret)
            error("An error occured while closing socket; errno = "
                      << errno << " : " << strerror(errno) );
    }

    return 0;
}

void* rxThread(void* args_)
{
    debug2("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;
    int ret = 0;
    int &msgQueueId = args.thisPtr->msgQueueIds[id];
    int connectionSocket = -1;
    struct sockaddr_in rxAddr;

    // init IPC //////////////////
    switch(args.thisPtr->ipcType)
    {
        case ipcSocket:
        {
            memset(&rxAddr, 0, sizeof(struct sockaddr_in) );

            connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);
            if (connectionSocket < 0)
               error("An error occured while opening socket");

            rxAddr.sin_family      = AF_INET;
            rxAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            rxAddr.sin_port        = htons( args.thisPtr->rxPorts[id] );
            debug2("RX_PORT: " << args.thisPtr->rxPorts[id] );

            ret = bind( connectionSocket, (struct sockaddr *) &rxAddr, sizeof(rxAddr) );
            if (0 != ret)
                error("An error occured while performing binding; errno = " << errno
                      << ": " << strerror(errno) );

            debug2("before post");
            ret = sem_post( &args.thisPtr->semaphores[id] );
            if (0 != ret)
                error("An error occured while performing sem_post; errno = " << errno
                      << ": " << strerror(errno) );
            debug2("after post");
            break;
        }

        default:
            break;
    }


    //prepare data ot send
    int dataSize = args.thisPtr->dataSize;
    char buf[dataSize];

    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {
        debug2("args.thisPtr = " << args.thisPtr);
        memset(buf, 0, dataSize);

        switch(args.thisPtr->ipcType)
        {
        case ipcMessageQueue:
        {
            debug2("ipcMessageQueue");
            //receive message
            msgData msg;
            memset(msg.data, 0, dataSize);

            debug2("args.thisPtr = " << args.thisPtr);

            ret = sem_wait( &args.thisPtr->semaphores[id] );
            if(0 != ret)
                error("An error occured while waiting on semaphore; timeout expired");

            debug2("msgQueueId = " << msgQueueId);
            ret = msgrcv(msgQueueId, &msg, dataSize, 1, 0);

            if(-1 == ret)
            error("An error occured while performing msgrcv; errno = "
                      << errno << ": " << strerror(errno) );

            debug("args.thisPtr = " << args.thisPtr);
            debug("args.thisPtr = " << args.thisPtr);
            debug("args.thisPtr->msgQuantity = " << args.thisPtr->msgQuantity);

            memcpy(buf, msg.data, dataSize);

            break;
        }

        case ipcPipe:
        {
            debug2("ipcPipe");

            ret = sem_wait( &args.thisPtr->semaphores[id] );

            if(0 != ret)
                error("An error occured while waiting on semaphore; timeout expired");

            ret = read(args.thisPtr->pipes[id][0], buf, dataSize);
            if(-1 == ret)
                error("An error occured while reading from pipe; errno = "
                          << errno << ": " << strerror(errno) );

            buf[dataSize-1] = '\0';

            debug("Message \"" << buf << "\" was received  {PIPE}");

            break;
        }

        case ipcSocket:
        {
            ret = recv(connectionSocket, buf, dataSize, MSG_WAITALL);

            if (-1 == ret)
                error("An error occured while performing receive; errno = " << errno
                      << ": " << strerror(errno) );



            debug("Message \"" << buf << "\" was received {SOCKET}");

            break;
        }

        default:
            debug2("default");
            break;
        }

        debug2("args.thisPtr = " << args.thisPtr);
        info("Message \"" << buf << "\" was received by [RX " << id << "]");
    }

    secondTimestamp(args, id);


    switch(args.thisPtr->ipcType)
    {
        case ipcMessageQueue:
        {
            ret = msgctl(args.thisPtr->msgQueueIds[id], IPC_RMID, NULL);
            if (0 != ret)
            error("An error occured while closing message queue; errno = "
                      << errno << " : " << strerror(errno) );
            break;
        }

        case ipcPipe:
        {
            ret = close( args.thisPtr->pipes[id][0] );
            if(0 != ret)
            error("An error occured while closing pipe; errno = "
                      << errno << ": " << strerror(errno) );

            ret = close( args.thisPtr->pipes[id][1] );
            if(0 != ret)
            error("An error occured while closing pipe; errno = "
                      << errno << ": " << strerror(errno) );
            break;
        }

        case ipcSocket:
        {
            ret = close(connectionSocket);
            if (0 != ret)
            error("An error occured while closing socket; errno = "
                      << errno << " : " << strerror(errno) );

            break;
        }

        default:
            break;
    }

    ret = sem_destroy( &args.thisPtr->semaphores[id] );
    if (0 != ret)
        error("An error occured while closing semaphore; errno = "
                  << errno << " : " << strerror(errno) );

    return 0;
}


void* generateOverload(void *overloadOn_)
{
    bool &overloadOn = * (bool*) overloadOn_;

    if(!overloadOn)
        return 0;

    info("Overload simulation is enabled");

    while(overloadOn)
        double v = (rand() * rand() << 3) / ( pow(rand(),1.431213) );

    info("Overload simulation is disabled");

    return 0;
}

testEnv::testEnv()
{
    threadsQuantity = 0;
    dataSize        = globDataSize;
    msgQuantity     = 0;
    semaphores      = 0;
    overloadOn      = false;
    ipcType         = ipcWrongValue;

    txThreads       = NULL;
    rxThreads       = NULL;
    msgQueueIds     = NULL;
    pipes           = NULL;
}

testEnv::~testEnv()
{
    debug2("");

    if(ipcPipe == ipcType)
    {
        for(int i=0; i<threadsQuantity; ++i)
            delete[] pipes[i];
    }

    for(int i=0; i<threadsQuantity; ++i)
        delete[] timeResults[i];

    delete[] timeResults;

    delete[] txPorts;
    delete[] rxPorts;
    delete[] txThreads;
    delete[] rxThreads;
    delete[] args;
    delete[] msgQueueIds;
    delete[] semaphores;
    delete[] pipes;
}

void testEnv::initIpc()
{
    debug2("");

    int ret = 0;

    if(NULL != msgQueueIds)
        delete[] msgQueueIds;

    if(NULL != pipes)
        delete[] pipes;

    rxPorts = new int[threadsQuantity];
    txPorts = new int[threadsQuantity];

    msgQueueIds = new int[threadsQuantity];
    pipes       = new int*[threadsQuantity];
    semaphores  = new sem_t[threadsQuantity];
    args        = new threadArgs[threadsQuantity];

    timeResults = new uint64_t*[threadsQuantity];
    for(int i=0; i<threadsQuantity; ++i)
        timeResults[i] = new uint64_t[2];

    for(int i=0; i<threadsQuantity; ++i)
    {
        rxPorts[i] = RX_PORT + i;
        txPorts[i] = TX_PORT + i;

        sem_destroy( &semaphores[i] );
        ret = sem_init(&semaphores[i], 0, 0);
        if(0 != ret)
            error("An error occured while writing to pipe; errno = "
                      << errno << ": " << strerror(errno) );
    }
}

void testEnv::initThreads()
{
    debug2("");

    if(NULL != txThreads)
        delete[] txThreads;

    if(NULL != rxThreads)
        delete[] rxThreads;

    txThreads   = new pthread_t[threadsQuantity];
    rxThreads   = new pthread_t[threadsQuantity];
}

void testEnv::createAndStartThreads()
{
    debug2("");

    int ret = -1;

    //start overload thread
    ret = pthread_create(&overloadThread, 0,
                                     generateOverload, &overloadOn);
    if(0 != ret)
        error("An error occurred while creating new thread;"
              << " ret = " << ret)


    for(int i=0; i<threadsQuantity; ++i)
    {


        args[i].threadNumber   = i;
        args[i].thisPtr        = this;


        debug2("args.threadNumber = " << args[i].threadNumber);

        //startTxThread
        ret = pthread_create(&txThreads[i], 0, txThread, &args[i]);
        if(0 != ret)
            error("An error occurred while creating new thread;"
                      << " ret = " << ret  << ": " << strerror(ret) );

        //startRxThread
        ret = pthread_create(&rxThreads[i], 0, rxThread, &args[i]);
        if(0 != ret)
            error("An error occurred while creating new thread;"
                      << " ret = " << ret << ": " << strerror(ret) );

    }

    debug2("Start joining");

    for(int i=0; i<threadsQuantity; ++i)
    {
        ret = pthread_join(txThreads[i], NULL);
        if(0 != ret)
            error("An error occurred while joining thread;"
                      << " ret = " << ret  << ": " << strerror(ret) );

        ret = pthread_join(rxThreads[i], NULL);
        if(0 != ret)
            error("An error occurred while joining thread;"
                      << " ret = " << ret  << ": " << strerror(ret) );
    }

    overloadOn = false;
    ret = pthread_join(overloadThread, NULL);
    if(0 != ret)
        error("An error occurred while joining thread;"
                  << " ret = " << ret  << ": " << strerror(ret) );
}

void testEnv::startTest()
{
    debug2("");

    info("Opening test environment");

    printTestEnvInfo();

    initIpc();
    initThreads();
    createAndStartThreads();

    info("Closing test environment");
}




uint64_t **testEnv::getTimeResults() const
{
    return timeResults;
}

void testEnv::setTimeResults(uint64_t **value)
{
    timeResults = value;
}
void testEnv::printTestEnvInfo()
{
    debug2("");

    const char *overloadOnStr = overloadOn ? "true" : "false";

    std::cout << "\t *--------- Test environment --------*" << std::endl
              << "\t  threadsQuantity:  " << threadsQuantity << std::endl
              << "\t  msgSize: \t    " << dataSize << std::endl
              << "\t  msgQuantity: \t    " << msgQuantity << std::endl
              << "\t  overloadOn: \t    " << overloadOnStr << std::endl
              << "\t  ipcType: \t    ";

    switch(ipcType)
    {
    case ipcMessageQueue:
    {
        std::cout << "ipcMessageQueue" << std::endl;
        break;
    }

    case ipcPipe:
    {
        std::cout << "ipcPipe" << std::endl;
        break;
    }

    case ipcSocket:
    {
        std::cout << "ipcSocket" << std::endl;
        break;
    }

    default:
    {
        std::cout << "ipcWrongValue" << std::endl;
        break;
    }

    }

    std::cout << "\t *-----------------------------------*" << std::endl;
}



int testEnv::getThreadsQuantity() const
{
    debug2("");

    return threadsQuantity;
}

void testEnv::setThreadsQuantity(int value)
{
    debug2("");

    threadsQuantity = value;
}

int testEnv::getDataSize() const
{
    debug2("");

    return dataSize;
}

void testEnv::setDataSize(int value)
{
    debug2("");

    dataSize = globDataSize;
}

int testEnv::getMsgQuantity() const
{
    debug2("");

    return msgQuantity;
}

void testEnv::setMsgQuantity(int value)
{
    debug2("");

    msgQuantity = value;
}

bool testEnv::getOverloadOn() const
{
    debug2("");

    return overloadOn;
}

void testEnv::setOverloadOn(bool value)
{
    debug2("");

    overloadOn = value;
}

ipcMechanismType testEnv::getIpcType() const
{
    debug2("");

    return ipcType;
}

void testEnv::setIpcType(const ipcMechanismType &value)
{
    debug2("");

    ipcType = value;
}


int *testEnv::getMsgQueueIds() const
{
    return msgQueueIds;
}

void testEnv::setMsgQueueIds(int *value)
{
    msgQueueIds = value;
}

int **testEnv::getPipes() const
{
    return pipes;
}

void testEnv::setPipes(int **value)
{
    pipes = value;
}
