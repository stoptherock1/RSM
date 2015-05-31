#include "testEnv.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define info(arg)   { \
                        pthread_mutex_lock(&lock); \
                        __uint64_t tid = 0; \
                        pthread_threadid_np(pthread_self(), &tid); \
                        std::cout << "[ INFO  | tid: " << tid << " ] : " \
                                  << arg << std::endl; \
                        pthread_mutex_unlock(&lock);\
                    }

#define error(arg)  { \
                        pthread_mutex_lock(&lock); \
                        __uint64_t tid = 0; \
                        pthread_threadid_np(pthread_self(), &tid); \
                        std::cout << "\e[0;31m[ ERROR | tid: " << tid \
                                  << " ] : \e[0m" << __FUNCTION__ \
                                  << " (line: " << __LINE__ << ") : " << arg \
                                  << std::endl; \
                        pthread_mutex_unlock(&lock); \
                    }


#define DEBUG_PRINTS
//#define DEEP_DEBUG_PRINTS

#ifdef DEBUG_PRINTS
    #define debug(arg)  { \
                            pthread_mutex_lock(&lock); \
                            __uint64_t tid = 0; \
                            pthread_threadid_np(pthread_self(), &tid); \
                            std::cout << "\e[0;36m[ DEBUG | tid: " << tid \
                                      << " ] : " << __FUNCTION__ \
                                      << " (line: " << __LINE__ << ") : " << arg \
                                      << "\e[0m" << std::endl; \
                            pthread_mutex_unlock(&lock); \
                        }
#else
    #define debug(arg)
#endif


#ifdef DEEP_DEBUG_PRINTS
    #define debug2(arg)  { \
                            pthread_mutex_lock(&lock); \
                            __uint64_t tid = 0; \
                            pthread_threadid_np(pthread_self(), &tid); \
                            std::cout << "\e[0;36m[ DEBUG | tid: " << tid \
                                      << " ] : " << __FUNCTION__ \
                                      << " (line: " << __LINE__ << ") : " << arg \
                                      << "\e[0m" << std::endl; \
                            pthread_mutex_unlock(&lock); \
                        }
#else
    #define debug2(arg)
#endif

#define PORT 18002

void *txThread(void* args_)
{
    debug("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;

    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {
        //prepare data ot send
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
            write(pipe, buf, dataSize);
            break;
        }

        case ipcSocket:
        {
            args.thisPtr->socketSnd(buf);
            break;
        }

        default:
        {
            break;
        }
        }

        info("Message \"" << buf << "\" was sent by [TX "
                << id << "]");
    }

    return 0;
}

void* rxThread(void* args_)
{
    debug("");

    threadArgs args = * (threadArgs*) args_;
    int id = args.threadNumber;

    for(int i=0; i<args.thisPtr->msgQuantity; ++i)
    {

        //prepare data ot send
        int msgSize = args.thisPtr->msgSize;
        char buf[msgSize];

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

            read(pipe, buf, msgSize);
            buf[msgSize-1] = '\0';

            break;
        }

        case ipcSocket:
        {
            debug2("ipcSocket");
            args.thisPtr->socketRcv(buf);
            break;
        }

        default:
            debug2("default");
            break;
        }
        //        msgrcv(args.msgQueueId, &msg, MSG_SIZE, 1, 0);

        info("Message \"" << buf << "\" was received by [RX " << id << "]")
    }

    return 0;
}

void testEnv::socketRcv(char *buf)
{
    debug("");

    static int i = 0;


    int connectionSocket = -1;
    int rcvSocket = -1;
    socklen_t txThreadAddrLen = -1;
    struct sockaddr_in rxThreadAddr, txThreadAddr;
    int n = -1;
    int ret = -1;

    memset(&rxThreadAddr, 0, sizeof(struct sockaddr_in) );
    memset(&txThreadAddr, 0, sizeof(struct sockaddr_in) );

    connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connectionSocket < 0)
       error("An error occured while opening socket");

    bzero( (char *) &rxThreadAddr, sizeof(rxThreadAddr) ) ;
    rxThreadAddr.sin_family = AF_INET;
    rxThreadAddr.sin_addr.s_addr = INADDR_ANY;
    rxThreadAddr.sin_port = htons( PORT + (++i) );

    ret = bind( connectionSocket, (struct sockaddr *) &rxThreadAddr, sizeof(rxThreadAddr) );
    if (0 != ret)
        error("An error occured while performing binding; errno = " << errno
              << ": " << strerror(errno) );

    debug2("listening");

    listen(connectionSocket, 5);

    txThreadAddrLen = sizeof(txThreadAddr);

    debug2("accepting");

    rcvSocket = accept(connectionSocket,
                       (struct sockaddr *) &txThreadAddr,
                       &txThreadAddrLen);
    if (rcvSocket < 0)
         error("An error occured while performing accept");

    debug2("receiving");

    bzero(buf, msgSize);
    n = read( rcvSocket, buf, msgSize );
    if (n < 0)
        error("An error occured while reading from socket");

    debug("Message \"" << buf << "\" was received");

    close(rcvSocket);
    close(connectionSocket);
}

void testEnv::socketSnd(char *buf)
{
    debug("");

    static int i = 0;

    int connectionSocket = -1;
    socklen_t txThreadAddrLen = 0;
    struct sockaddr_in rxThreadAddr, txThreadAddr;
    int ret = -1;

    memset(&rxThreadAddr, 0, sizeof(struct sockaddr_in) );
    memset(&txThreadAddr, 0, sizeof(struct sockaddr_in) );

    connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connectionSocket < 0)
       error("An error occured while opening socket");


    bzero( (char *) &rxThreadAddr, sizeof(rxThreadAddr) ) ;
    rxThreadAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    rxThreadAddr.sin_family = AF_INET;
    rxThreadAddr.sin_port = htons( PORT + (++i) );

    ret = connect( connectionSocket,
                   (struct sockaddr *)&rxThreadAddr,
                   sizeof(rxThreadAddr) );
    if (-1 == ret)
        error("An error occured while performing connect; errno = "
                  << errno << ": " << strerror(errno) );

    ret = send(connectionSocket, buf, strlen(buf), 0);
    if (-1 == ret)
        error("An error occured while performing send; errno = "
                  << errno << " : " << strerror(errno) );
    debug("Message \"" << buf << "\" was sent");

    close(connectionSocket);
}

void* generateOverload(void *overloadOn_)
{
    bool overloadOn = * (bool*) overloadOn_;

    if(!overloadOn)
        return 0;

    info("Overload simulated is enabled");

    while(overloadOn)
        double v = (rand() * rand() << 3) / ( pow(rand(),1.431213) );

    info("Overload simulated is disabled");

    return 0;
}

testEnv::testEnv()
{
    threadsQuantity = 0;
    msgSize         = 0;
    msgQuantity     = 0;
    overloadOn      = false;
    ipcType         = ipcWrongValue;

    txThreads       = NULL;
    rxThreads       = NULL;
    msgQueueIds     = NULL;
    pipes           = NULL;

    if(pthread_mutex_init(&lock, NULL) != 0)
        error("Mutex init failed");

}

testEnv::~testEnv()
{
    debug("");

    for(int i=0; i<threadsQuantity; ++i)
        msgctl(msgQueueIds[i], IPC_RMID, NULL);

    overloadOn = false;
    delete[] txThreads;
    delete[] rxThreads;
    delete[] msgQueueIds;

    for(int i=0; i<threadsQuantity; ++i)
    {
        close( pipes[i][0] );
        close( pipes[i][1] );
        delete[] pipes[i];
    }
    delete[] pipes;
}

void testEnv::initIpc()
{
    debug("");

    if(NULL != msgQueueIds)
        delete[] msgQueueIds;

    if(NULL != pipes)
        delete[] pipes;

    msgQueueIds = new int[threadsQuantity];
    pipes       = new int*[threadsQuantity];
    for(int i=0; i<threadsQuantity; ++i)
    {
        pipes[i] = new int[2];
        pipe( pipes[i] );
    }
}

void testEnv::createThreads()
{
    debug("");

    if(NULL != txThreads)
        delete[] txThreads;

    if(NULL != rxThreads)
        delete[] rxThreads;

    txThreads   = new pthread_t[threadsQuantity];
    rxThreads   = new pthread_t[threadsQuantity];
}

void testEnv::startThreads()
{
    debug("");

    int threadCreateRet = -1;

    //start overload thread
    threadCreateRet = pthread_create(&overloadThread, 0,
                                     generateOverload, &overloadOn);
    if(0 != threadCreateRet)
        error("An error occurred while creating new thread;" << " threadCreateRet = " << threadCreateRet)
    else
        pthread_join(overloadThread, NULL);


    for(int i=0; i<threadsQuantity; ++i)
    {
        //create msg queue
        key_t key = ftok("test", i);
        msgQueueIds[i] = msgget(key, 0644 | IPC_CREAT);

        threadArgs args;
        args.threadNumber   = i;
        args.thisPtr        = this;

        if(ipcSocket == ipcType)    // in case of socket
        {                           // listerner should be started first
            startRxThread(i, args);
            startTxThread(i, args);
            //give some time for the detached threads to do their work
            sleep(1);
        }
        else
        {
            startTxThread(i, args);
            startRxThread(i, args);
        }

    }
}
void testEnv::startRxThread(int i, threadArgs args)
{
    debug("");

    int threadCreateRet = pthread_create(&rxThreads[i], 0,
                                     rxThread, &args);
    if(0 != threadCreateRet)
    {
        error("An error occurred while creating new thread;"
                  << " threadCreateRet = " << threadCreateRet);
        return;
    }

    if(ipcSocket == ipcType)
        pthread_detach(rxThreads[i]);
    else
        pthread_join(rxThreads[i], NULL);
}

void testEnv::startTxThread(int i, threadArgs args)
{
    debug("");

    int threadCreateRet = pthread_create(&txThreads[i], 0,
                                     txThread, &args);
    if(0 != threadCreateRet)
        error("An error occurred while creating new thread;"
                  << " threadCreateRet = " << threadCreateRet)
    else
        pthread_join(txThreads[i], NULL);
//        pthread_detach(txThreads[i]);
}

void testEnv::createAndRunThreads()
{
    debug("");

    info("Opening test environment");

    printTestEnvInfo();

    initIpc();
    createThreads();
    startThreads();

    info("Closing test environment");
}

void testEnv::printTestEnvInfo()
{
    debug("");

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
    debug("");

    return threadsQuantity;
}

void testEnv::setThreadsQuantity(int value)
{
    debug("");

    threadsQuantity = value;
}

int testEnv::getMsgSize() const
{
    debug("");

    return msgSize;
}

void testEnv::setMsgSize(int value)
{
    debug("");

    msgSize = value;
}

int testEnv::getMsgQuantity() const
{
    debug("");

    return msgQuantity;
}

void testEnv::setMsgQuantity(int value)
{
    debug("");

    msgQuantity = value;
}

bool testEnv::getOverloadOn() const
{
    debug("");

    return overloadOn;
}

void testEnv::setOverloadOn(bool value)
{
    debug("");

    overloadOn = value;
}

ipcMechanismType testEnv::getIpcType() const
{
    debug("");

    return ipcType;
}

void testEnv::setIpcType(const ipcMechanismType &value)
{
    debug("");

    ipcType = value;
}
