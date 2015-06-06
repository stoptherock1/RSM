#include "testEnv.hpp"

pthread_mutex_t stdOutLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bindLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ioLock = PTHREAD_MUTEX_INITIALIZER;


#define RX_PORT 18000
#define TX_PORT 28000
#define OSX
#define DEBUG_PRINTS
//#define DEEP_DEBUG_PRINTS


#define info(arg)   { \
                        pthread_mutex_lock(&stdOutLock); \
                        std::cout << "[ INFO ]  : " \
                                  << "txThread (line:  124) : " \
                                  << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                  << arg << std::endl; \
                        pthread_mutex_unlock(&stdOutLock);\
                    }

#define error(arg)  { \
                        pthread_mutex_lock(&stdOutLock); \
                        std::cout << "\e[0;31m[ ERROR ]\e[0m : " \
                                  << __FUNCTION__\
                                  << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                  << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                  << arg \
                                  << std::endl; \
                        pthread_mutex_unlock(&stdOutLock); \
                    }



#ifdef DEBUG_PRINTS
    #define debug(arg)  { \
                            pthread_mutex_lock(&stdOutLock); \
                            std::cout << "\e[0;36m[ DEBUG ] : " \
                                      << __FUNCTION__ \
                                      << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                      << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                      << arg \
                                      << "\e[0m" << std::endl; \
                            pthread_mutex_unlock(&stdOutLock); \
                        }
#else
    #define debug(arg)
#endif

#ifdef DEEP_DEBUG_PRINTS
    #define debug2(arg)  { \
                            pthread_mutex_lock(&stdOutLock); \
                            std::cout << "\e[0;36m[ DEBUG ] : " \
                                      << __FUNCTION__ \
                                      << " (line: " << std::setw(4) << __LINE__ << ") : " \
                                      << "tid: " << std::hex << tid() << " : " <<  std::dec\
                                      << arg \
                                      << "\e[0m" << std::endl; \
                            pthread_mutex_unlock(&stdOutLock); \
                        }
#else
    #define debug2(arg)
#endif

#ifdef OSX
    __uint64_t tid()
    {
        __uint64_t tid = 0;
        pthread_threadid_np(pthread_self(), &tid);
        return tid;
    }
#else

#endif


void *txThread(void* args_)
{
    debug2("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;

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

            pthread_mutex_lock(&ioLock);

            int ret = write(pipe, buf, dataSize);
            if(-1 == ret)
            {
                error("An error occured while writing to pipe; errno = "
                          << errno << ": " << strerror(errno) );
                return 0;
            }

            debug("Message \"" << buf << "\" was sent  {PIPE}");
            pthread_mutex_unlock(&ioLock);

           ret = dispatch_semaphore_signal( args.thisPtr->semaphores[id] );
            if(0 == ret)
                error("An error occured while waking a thread");
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


            ret = dispatch_semaphore_wait( args.thisPtr->semaphores[id] , DISPATCH_TIME_FOREVER);
            if(0 != ret)
                error("An error occured while waiting on semaphore; timeout expired");

            debug(id << " is reading");

            pthread_mutex_lock(&ioLock);

            ret = read(pipe, buf, msgSize);
            buf[msgSize-1] = '\0';

            close( args.thisPtr->pipes[id][0] );
            close( args.thisPtr->pipes[id][1] );

            if(-1 == ret)
            {
                error("An error occured while reading from pipe; errno = "
                          << errno << ": " << strerror(errno) );
                return 0;
            }

            debug("Message \"" << buf << "\" was received  {PIPE}");
            pthread_mutex_unlock(&ioLock);

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

    pthread_mutex_lock(&bindLock);
    txAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    txAddr.sin_family      = AF_INET;
    txAddr.sin_port        = htons(TX_PORT + i);
    debug("TX_PORT: " << TX_PORT + (++j) );

    rxAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    rxAddr.sin_family      = AF_INET;
    rxAddr.sin_port        = htons(RX_PORT + i);
    debug("RX_PORT: " << RX_PORT + (++i) );
    pthread_mutex_unlock(&bindLock);

    ret = bind( connectionSocket, (struct sockaddr *) &txAddr, sizeof(txAddr) );

    debug("before wait");
    dispatch_semaphore_wait( semaphores[i] , DISPATCH_TIME_FOREVER);
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

    close(connectionSocket);
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

    pthread_mutex_lock(&bindLock);
    rxAddr.sin_family      = AF_INET;
    rxAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rxAddr.sin_port        = htons(RX_PORT + i);
    debug("RX_PORT: " << RX_PORT + (++i) );
    pthread_mutex_unlock(&bindLock);

    ret = bind( connectionSocket, (struct sockaddr *) &rxAddr, sizeof(rxAddr) );
    if (0 != ret)
        error("An error occured while performing binding; errno = " << errno
              << ": " << strerror(errno) );

    debug("before post");
    dispatch_semaphore_signal( semaphores[i] );
    debug("after post");

    ret = recv(connectionSocket, buf, msgSize, MSG_WAITALL);
    if (-1 == ret)
        error("An error occured while performing receive; errno = " << errno
              << ": " << strerror(errno) );

    debug("Message \"" << buf << "\" was received {SOCKET}");

    close(connectionSocket);
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

    delete[] txThreads;
    delete[] rxThreads;
    delete[] msgQueueIds;
    delete[] args;

    for(int i=0; i<threadsQuantity; ++i)
    {
        msgctl(msgQueueIds[i], IPC_RMID, NULL);
        delete[] pipes[i];
        dispatch_release( semaphores[i] );
    }

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
    semaphores  = new dispatch_semaphore_t[threadsQuantity];
    args        = new threadArgs[threadsQuantity];

    for(int i=0; i<threadsQuantity; ++i)
    {
        pipes[i] = new int[2];
        int ret = pipe( pipes[i] );
        if(-1 == ret)
            error("An error occured while writing to pipe; errno = "
                      << errno << ": " << strerror(errno) );

        semaphores[i] = dispatch_semaphore_create(0);

        if(NULL == semaphores[i])
            error("An error occured while creating semaphore");
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

    int threadCreateRet = -1;

    //start overload thread
    threadCreateRet = pthread_create(&overloadThread, 0,
                                     generateOverload, &overloadOn);
    if(0 != threadCreateRet)
        error("An error occurred while creating new thread;"
              << " threadCreateRet = " << threadCreateRet)


    for(int i=0; i<threadsQuantity; ++i)
    {
        //create msg queue
        key_t key = ftok("test", i);
        msgQueueIds[i] = msgget(key, 0644 | IPC_CREAT);

        args[i].threadNumber   = i;
        args[i].thisPtr        = this;

        debug2("args.threadNumber = " << args[i].threadNumber);

        //startTxThread
        threadCreateRet = pthread_create(&txThreads[i], 0,
                                         txThread, &args[i]);
        if(0 != threadCreateRet)
            error("An error occurred while creating new thread;"
                      << " threadCreateRet = " << threadCreateRet)

        //startRxThread
        threadCreateRet = pthread_create(&rxThreads[i], 0,
                                         rxThread, &args[i]);
        if(0 != threadCreateRet)
            error("An error occurred while creating new thread;"
                      << " threadCreateRet = " << threadCreateRet);

    }

    info("Start joining");

    for(int i=0; i<threadsQuantity; ++i)
    {
        pthread_join(txThreads[i], NULL);
        pthread_join(rxThreads[i], NULL);
    }

    overloadOn = false;
    pthread_join(overloadThread, NULL);
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
