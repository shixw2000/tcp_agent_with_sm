#ifndef __MANAGECENTER_H__
#define __MANAGECENTER_H__
#include"globaltype.h"
#include"listnode.h"
#include"datatype.h"


struct FdInfo;
struct MsgHdr;

class PollPool;
class SockDealer;
class UserCenter;

class UsrAccptAuth;
class UsrConnAuth;
class SessAccptAuth;
class SessConnAuth;
class AdminAccptAuth;
class AdminConnAuth;

class ManageCenter {  
    typedef Int32 (ManageCenter::*PReader)(FdInfo*);
    typedef Int32 (ManageCenter::*PWriter)(FdInfo*);
    typedef Void (ManageCenter::*PDealer)(FdInfo*, MsgHdr*);
    
public:
    ManageCenter();
    ~ManageCenter(); 

    Int32 init();
    Void finish();

    Void set(SockDealer* dealer);

    Int32 start();
    Void stop();
    Void wait(); 

    Int32 readInfo(FdInfo*);
    Int32 writeInfo(FdInfo*);
    Int32 failInfo(FdInfo*);

    Void closeFd(FdInfo* info);

    Int32 procMsg(FdInfo* info, MsgHdr* msg);
    
    Int32 dispatchMsg(FdInfo* info, MsgHdr* msg);
    
    Int32 sendMsg(FdInfo* info, MsgHdr* msg); 


    Int32 startUsrConn(const TcpParam* param, UserConn* user);

    Int32 startSessConn(const TcpParam* param, Uint32 sessID,
        UserAccpt* parent);

    Int32 addListener(const TcpPairs* pairs);

private:
    template<Int32 type>
    Int32 readFd(FdInfo*);

    template<Int32 type>
    Int32 writeFd(FdInfo*);

    template<Int32 type>
    Void dealFd(FdInfo*, MsgHdr*);
    
    Int32 addTcpListener(const TcpPairs* pairs);
    
    Int32 addDirtyListener(const TcpPairs* pairs);

    Int32 addAdminListener(const TcpPairs* pairs);

    Int32 addUser(ListenerDirty* listener, Int32 new_fd);
    Int32 addSess(ListenerTcp* listener, Int32 new_fd);
    Int32 addAdmin(ListenerAdmin* listener, Int32 new_fd);
  
private:     
    static const PReader m_rd_funcs[ENUM_RD_END];
    static const PWriter m_wr_funcs[ENUM_WR_END];
    static const PDealer m_deal_funcs[ENUM_DEAL_END];

    list_head m_listener_list;
    
    PollPool* m_poll_pool;
    UserCenter* m_usr_center;

    SockDealer* m_dealer;

    UsrAccptAuth* m_deal_usr_accpt;
    UsrConnAuth* m_deal_usr_conn;
    SessAccptAuth* m_deal_sess_accpt;
    SessConnAuth* m_deal_sess_conn;
    AdminAccptAuth* m_deal_admin_accpt;
    AdminConnAuth* m_deal_admin_conn;

    Uint32 m_last_usr_id;
};

#endif

