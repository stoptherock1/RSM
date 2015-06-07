#include "testEnv.hpp"

pthread_mutex_t stdOutLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bindLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ioLock = PTHREAD_MUTEX_INITIALIZER;


#define RX_PORT 18000
#define TX_PORT 28000
#define DEBUG_PRINTS
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



void *txThread(void* args_)
{
    debug2("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;
    int ret =  0;

    debug2("id = " << id);

    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {
        //prepare data to send
        int dataSize = args.thisPtr->msgSize;
        char buf[dataSize];

        memset(buf, 0, dataSize);
        snprintf(buf, dataSize, "tx %d MESSAGE %d", id, i);
//        snprintf(buf, dataSize, "tx");
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
            msg.data = (char*) calloc( dataSize, sizeof(char) );

            //send message
            msgsnd(args.msgQueueId, &msg, dataSize, msgType);

            free(msg.data);
            break;
        }

        case ipcPipe:
        {
            int &pipe = args.thisPtr->pipes[id][1]; //pipe[0] - read, pipe[1] - write

            ret = pthread_mutex_lock(&ioLock);
            if(0 != ret)
                error("An error occured while locking mutex; ret = "
                      << ret << ": " << strerror(ret) );

            ret = write(pipe, buf, dataSize);
            if(-1 == ret)
                error("An error occured while writing to pipe; errno = "
                          << errno << ": " << strerror(errno) );

            debug("Message \"" << buf << "\" was sent  {PIPE}");
            ret = pthread_mutex_unlock(&ioLock);
            if(0 != ret)
                error("An error occured while unlocking mutex; ret = "
                      << ret << ": " << strerror(ret) );

           ret = sem_post( &args.thisPtr->semaphores[id] );
            if(0 != ret)
                error("An error occured while waking a thread; errno = "
                      << errno << ": " << strerror(errno) );
            debug(id << " writed");

            break;
        }

        case ipcSocket:
        {
            args.thisPtr->txSocket(buf, args.thisPtr);
            break;
        }

        default:
        {
            break;
        }
        }

//        info("Message \"" << buf << "\" was sent by [TX "
//                << id << "]");
    }

    return 0;
}

void* rxThread(void* args_)
{
    debug2("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;
    int ret = 0;

    debug2("id = " << id);

    //prepare data ot send
    int msgSize = args.thisPtr->msgSize;
    char buf[msgSize];

    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {
        memset(buf, 0, msgSize);

        switch(args.thisPtr->ipcType)
        {
        case ipcMessageQueue:
        {
            debug2("ipcMessageQueue");
            //receive message
            msgData msg;
            msg.data = (char*) calloc( msgSize, sizeof(char) );
            memset(msg.data, 0, msgSize);
            //           msgrcv(args.msgQueueId, &msg, MSG_SIZE, msgType);
            free(msg.data);
            break;
        }

        case ipcPipe:
        {
            debug2("ipcPipe");
            int &pipe = args.thisPtr->pipes[id][0]; //pipe[0] - read, pipe[1] - write


            ret = sem_wait( &args.thisPtr->semaphores[id] );
            if(0 != ret)
                error("An error occured while waiting on semaphore; timeout expired");

            debug(id << " is reading");

            ret = pthread_mutex_lock(&ioLock);
            if(0 != ret)
                error("An error occured while locking mutex; ret = "
                      << ret << ": " << strerror(ret) );

            ret = read(pipe, buf, msgSize);
            buf[msgSize-1] = '\0';

            ret = close( args.thisPtr->pipes[id][0] );
            error("An error occured while closing pipe; errno = "
                      << errno << ": " << strerror(errno) );

            ret = close( args.thisPtr->pipes[id][1] );
            error("An error occured while closing pipe; errno = "
                      << errno << ": " << strerror(errno) );

            if(-1 == ret)
            {
                error("An error occured while reading from pipe; errno = "
                          << errno << ": " << strerror(errno) );
            }

            debug("Message \"" << buf << "\" was received  {PIPE}");
            ret = pthread_mutex_unlock(&ioLock);
            if(0 != ret)
                error("An error occured while locking mutex; ret = "
                      << ret << ": " << strerror(ret) );

            break;
        }

        case ipcSocket:
        {
            debug2("ipcSocket");
            args.thisPtr->rxSocket(buf, args.thisPtr);
            break;
        }

        default:
            debug2("default");
            break;
        }
        //        msgrcv(args.msgQueueId, &msg, MSG_SIZE, 1, 0);

//        info("Message \"" << buf << "\" was received by [RX " << id << "]")
    }

    ret = sem_destroy( &args.thisPtr->semaphores[id] );
    if (0 != ret)
        error("An error occured while closing semaphore; errno = "
                  << errno << " : " << strerror(errno) );

    ret = sem_destroy( &args.thisPtr->semaphores[id] );
    if (0 != ret)
        error("An error occured while closing semaphore; errno = "
                  << errno << " : " << strerror(errno) );

    ret = msgctl(args.thisPtr->msgQueueIds[id], IPC_RMID, NULL);
    if (0 != ret)
        error("An error occured while closing message queue; errno = "
                  << errno << " : " << strerror(errno) );

    return 0;
}

void testEnv::txSocket(char *buf, testEnv* thisPtr)
{
    debug2("");

    int connectionSocket = -1;
    struct sockaddr_in rxAddr;
    struct sockaddr_in txAddr;
    int ret = -1;

    static int i = 0;
    static int j = 0;

    memset(&rxAddr, 0, sizeof(struct sockaddr_in) );

    connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (connectionSocket < 0)
       error("An error occured while opening socket");

    ret = pthread_mutex_lock(&bindLock);
    if(0 != ret)
        error("An error occured while locking mutex; ret = "
              << ret << ": " << strerror(ret) );

    txAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    txAddr.sin_family      = AF_INET;
    txAddr.sin_port        = htons(TX_PORT + i);
    debug("TX_PORT: " << TX_PORT + (++j) );

    rxAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    rxAddr.sin_family      = AF_INET;
    rxAddr.sin_port        = htons(RX_PORT + i);
    debug("RX_PORT: " << RX_PORT + (++i) );

    ret = pthread_mutex_unlock(&bindLock);
    if(0 != ret)
        error("An error occured while unlocking mutex; ret = "
              << ret << ": " << strerror(ret) );


    ret = bind( connectionSocket, (struct sockaddr *) &txAddr, sizeof(txAddr) );
    if (0 != ret)
        error("An error occured while performing bind; errno = "
                  << errno << ": " << strerror(errno) );

    debug("before wait");
    sem_wait( &semaphores[i] );
    if (0 != ret)
        error("An error occured while performing sem_wait; errno = "
                  << errno << ": " << strerror(errno) );
    debug("after wait");

    ret = connect( connectionSocket,
                   (struct sockaddr *)&rxAddr,
                   sizeof(rxAddr) );
    if (-1 == ret)
        error("An error occured while performing connect; errno = "
                  << errno << ": " << strerror(errno) );

    ret = send(connectionSocket, buf, strlen(buf), 0);
    if (-1 == ret)
        error("An error occured while performing send; errno = "
                  << errno << " : " << strerror(errno) );
    debug("Message \"" << buf << "\" was sent  {SOCKET}");

    ret = close(connectionSocket);
    if (0 != ret)
        error("An error occured while closing socket; errno = "
                  << errno << " : " << strerror(errno) );
}

void testEnv::rxSocket(char *buf, testEnv* thisPtr)
{
    debug2("");

    int connectionSocket = -1;
    struct sockaddr_in rxAddr;
    int ret = -1;

    static int i = 0;

    memset(&rxAddr, 0, sizeof(struct sockaddr_in) );

    connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (connectionSocket < 0)
       error("An error occured while opening socket");

    ret = pthread_mutex_lock(&bindLock);
    if(0 != ret)
        error("An error occured while locking mutex; ret = "
              << ret << ": " << strerror(ret) );

    rxAddr.sin_family      = AF_INET;
    rxAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rxAddr.sin_port        = htons(RX_PORT + i);
    debug("RX_PORT: " << RX_PORT + (++i) );

    ret = pthread_mutex_unlock(&bindLock);
    if(0 != ret)
        error("An error occured while unlocking mutex; ret = "
              << ret << ": " << strerror(ret) );

    ret = bind( connectionSocket, (struct sockaddr *) &rxAddr, sizeof(rxAddr) );
    if (0 != ret)
        error("An error occured while performing binding; errno = " << errno
              << ": " << strerror(errno) );

    debug("before post");
    sem_post( &semaphores[i] );
    if (0 != ret)
        error("An error occured while performing sem_post; errno = " << errno
              << ": " << strerror(errno) );
    debug("after post");

    ret = recv(connectionSocket, buf, msgSize, MSG_WAITALL);
    if (-1 == ret)
        error("An error occured while performing receive; errno = " << errno
              << ": " << strerror(errno) );

    debug("Message \"" << buf << "\" was received {SOCKET}");

    ret = close(connectionSocket);
    if (0 != ret)
        error("An error occured while closing socket; errno = "
                  << errno << " : " << strerror(errno) );
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
    msgSize         = 0;
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

    int ret = 0;

    delete[] txThreads;
    delete[] rxThreads;
    delete[] args;

    for(int i=0; i<threadsQuantity; ++i)
    {


        delete[] pipes[i];

        ret = sem_destroy( &semaphores[1] );
        if (0 != ret)
            error("An error occured while closing semaphore; errno = "
                      << errno << " : " << strerror(errno) );

    }

    delete[] msgQueueIds;
    delete[] semaphores;
    delete[] pipes;
}

void testEnv::initIpc()
{
    debug2("");

    if(NULL != msgQueueIds)
        delete[] msgQueueIds;

    if(NULL != pipes)
        delete[] pipes;

    msgQueueIds = new int[threadsQuantity];
    pipes       = new int*[threadsQuantity];
    semaphores  = new sem_t[threadsQuantity];
    args        = new threadArgs[threadsQuantity];

    for(int i=0; i<threadsQuantity; ++i)
    {
        pipes[i] = new int[2];
        int ret = pipe( pipes[i] );
        if(-1 == ret)
            error("An error occured while writing to pipe; errno = "
                      << errno << ": " << strerror(errno) );

        ret = sem_init(&semaphores[i], 0, 0);
        if(-1 == ret)
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
        //create msg queue
        key_t key = ftok("main.cpp", i);
        if(-1 == key)
            error("An error occured while performing ftok; errno = "
                      << errno << ": " << strerror(errno) );


        msgQueueIds[i] = msgget(key, 0644 | IPC_CREAT);

        args[i].threadNumber   = i;
        args[i].thisPtr        = this;

        debug2("args.threadNumber = " << args[i].threadNumber);

        //startTxThread
        ret = pthread_create(&txThreads[i], 0, txThread, &args[i]);
        if(0 != ret)
            error("An error occurred while creating new thread;"
                      << " ret = " << ret)

        //startRxThread
        ret = pthread_create(&rxThreads[i], 0, rxThread, &args[i]);
        if(0 != ret)
            error("An error occurred while creating new thread;"
                      << " ret = " << ret);

    }

    info("Start joining");

    for(int i=0; i<threadsQuantity; ++i)
    {
        ret = pthread_join(txThreads[i], NULL);
        if(0 != ret)
            error("An error occurred while joining thread;"
                      << " ret = " << ret);

        ret = pthread_join(rxThreads[i], NULL);
        if(0 != ret)
            error("An error occurred while joining thread;"
                      << " ret = " << ret);
    }

    overloadOn = false;
    ret = pthread_join(overloadThread, NULL);
    if(0 != ret)
        error("An error occurred while joining thread;"
                  << " ret = " << ret);
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

void testEnv::printTestEnvInfo()
{
    debug2("");

    const char *overloadOnStr = overloadOn ? "true" : "false";

    std::cout << "\t *--------- Test environment --------*" << std::endl
              << "\t  threadsQuantity:  " << threadsQuantity << std::endl
              << "\t  msgSize: \t    " << msgSize << std::endl
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

int testEnv::getMsgSize() const
{
    debug2("");

    return msgSize;
}

void testEnv::setMsgSize(int value)
{
    debug2("");

    msgSize = value;
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
