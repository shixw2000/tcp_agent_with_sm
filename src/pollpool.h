#ifndef __POLLPOOL_H__
#define __POLLPOOL_H__
#include"globaltype.h"
#include"listnode.h"
#include"cthread.h"


struct pollfd;
struct FdInfo;
struct SockBase;
struct MsgHdr;
struct EventData;
class ManageCenter; 
class UserCenter;
class Lock;
class SockOper; 

class PollPool : public CThread {    
public:
    PollPool();
    ~PollPool();

    Int32 init();
    Void finish(); 

    Void set(ManageCenter* mng, UserCenter* usr_center);

    FdInfo* creatReader(Int32 fd, Int32 fd_type, Void* io);
    
    FdInfo* creatSock(Int32 fd, Int32 fd_status,
        Int32 rd_type, Int32 wr_type,
        Int32 deal_type, Void* io, Void* data); 

    Void addEvent(FdInfo* info);

    Void resetFd(FdInfo* info);
    Void finishFd(FdInfo* info);

    Int32 readEventCmd(EventData* data);
    
    Int32 sendCmd(MsgHdr* msg);

    Int32 sendMsg(FdInfo* info, MsgHdr* msg); 
    Int32 writeData(FdInfo* info, SockBase* sock); 
    Int32 readData(FdInfo* info, SockBase* sock);
    Int32 readTcpRaw(FdInfo* info, SockBase* sock);

    Void signal();

private:
    virtual Int32 run(); 

    Int32 fillEvent();
    Int32 waitEvent(Int32 cnt); 

    Int32 pollEvent();

    Void delEvent(FdInfo* info); 
    
    Int32 procEventCmd(list_head* list);
    Void execCmd(MsgHdr* msg); 

    Bool lock();
    Bool unlock();

private: 
    const Int32 m_capacity;
    
    list_head m_lock_list;
    list_head m_run_list;
    list_head m_idle_list;
    
    ManageCenter* m_mng; 
    UserCenter* m_usr_center;
    
    struct pollfd* m_fds;
    FdInfo* m_infos;
    Lock* m_lock; 
    EventData* m_event_data;
    SockOper* m_sock_oper;
};

#endif

