#ifndef __USERCENTER_H__
#define __USERCENTER_H__
#include"globaltype.h"
#include"datatype.h"
#include"listnode.h"
#include"msgtype.h"


struct EvpBase;
class ManageCenter;

class UserCenter {    
public:
    explicit UserCenter(ManageCenter* mng);
    ~UserCenter();

    Void freeMsgQue(list_head* list);

    /* if close, delete item from que by the one itself for gracefully exit */
    Void freeUsrAccptQue(list_head* list, Bool isClose);
    Void freeUsrConnQue(list_head* list, Bool isClose);
    Void freeSessAccptQue(order_list_head* list, Bool isClose);
    Void freeSessConnQue(order_list_head* list, Bool isClose);
    Void freeAdminAccptQue(list_head* list, Bool isClose);

    template<typename T>
    T* creatData();

    template<typename T>
    Void freeData(T*);

    SessionAccpt* findSessAccpt(Uint32 sessID, UserConn* user);
    SessionConn* findSessConn(Uint32 sessID, UserAccpt* user);
        
    EkeyBase* creatEkeyBase();
    Void resetEkeyBase(EkeyBase* base);
    Void closeEkeyBase(EkeyBase* base); 
    
    Void closeSock(SockBase* sock);
    Void resetSock(SockBase* sock);

    Void resetNode(NodeBase* base);

    EvpBase* creatEvp();
    Void closeEvp(EvpBase* evp);
    Void freeEvp(EvpBase* evp);

    Int32 startAgent(FdInfo* info, Uint32 usrId, Uint32 sessId);
    Int32 stopAgent(FdInfo* info, Uint32 usrId, Uint32 sessId, Int32 reason);
    Int32 startAuth(UserConn* usrConn);

    MsgHdr* decCipherText(EvpBase* evp, MsgHdr* input);
    MsgHdr* encPlainText(EvpBase* evp, MsgHdr* input);

    Void setCipherKey(EvpBase* evp, const Void* usr_key, Int32 len);
    Int32 encrypt(EvpBase* evp, const Void* data, Int32 len, Void* out);
    Int32 decrypt(EvpBase* evp, const Void* data, Int32 len, Void* out);

    Int32 digest(EvpBase* evp, const Void* data, Int32 len, Void* out);

    Uint32 hashMac(EvpBase* evp, const Void* data, Int32 len);

private: 
    ManageCenter* m_mng;
    
};

#endif

