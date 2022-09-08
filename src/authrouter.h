#ifndef __AUTHROUTER_H__
#define __AUTHROUTER_H__
#include"globaltype.h"
#include"datatype.h"


struct MsgHdr;
class ManageCenter;
class UserCenter;

class RouterDealer {
public:
    RouterDealer(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 processIn(RouterPair* routerPair, MsgHdr* msg, Bool* pDel);
    Int32 processOut(RouterPair* routerPair, MsgHdr* msg, Bool* pDel);

private: 
    Int32 procCipherData(RouterPair* routerPair, Router* local,
        Router* peer, MsgHdr* msg, Bool* pDel); 
    
    Int32 procPlainData(RouterPair* routerPair, Router* local, 
        Router* peer, MsgHdr* msg, Bool* pDel); 
    
    Int32 procAuthReq(RouterPair* routerPair, Router* local, 
        Router* peer, MsgHdr* msg, Bool* pDel);
    
    Int32 procCipherKeyAck(RouterPair* routerPair, 
        Router* local, Router* peer, MsgHdr* msg, Bool* pDel); 
    
    Int32 procExchKey(RouterPair* routerPair, Router* local,
        Router* peer, MsgHdr* msg, Bool* pDel);

    Int32 procAuthEnd(RouterPair* routerPair, Router* local,
        Router* peer, MsgHdr* msg, Bool* pDel);

    Int32 procStopGateWay(RouterPair* routerPair, Router* local,
        Router* peer, MsgHdr* msg, Bool* pDel);

    Int32 procCloseRouter(RouterPair* routerPair, Router* local, 
        Router* peer, MsgHdr* msg, Bool* pDel);
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

