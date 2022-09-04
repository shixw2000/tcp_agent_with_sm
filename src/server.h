#ifndef __SERVER_H__
#define __SERVER_H__
#include"globaltype.h"
#include"listnode.h"


struct Parser;
class ManageCenter;
class SockDealer;
class CService;

class Server {
public: 
    Server();
    ~Server();

    Void set(const Char* path);

    Int32 init();
    Void finish();

    Int32 startServer();
    Void stopServer();
    Void wait();

private: 
    Int32 addTcpPairs(Int32 type, 
        const Char szLocalIp[], Int32 localPort, 
        const Char szPeerIp[], Int32 peerPort); 

private:
    Parser* m_parser;
    ManageCenter* m_mng;
    SockDealer* m_dealer;
    CService* m_deal_svc;
    const Char* m_path;
};

#endif

