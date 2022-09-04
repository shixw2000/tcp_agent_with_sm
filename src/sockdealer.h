#ifndef __SOCKDEALER_H__
#define __SOCKDEALER_H__
#include"globaltype.h"
#include"listnode.h"
#include"cthread.h"


struct FdInfo;
struct MsgHdr;
class ManageCenter;
class MutexCond;

class SockDealer : public I_Worker { 
public:
    explicit SockDealer(ManageCenter* mng);
    ~SockDealer();

    Int32 init();
    Void finish(); 

    Int32 dispatchMsg(FdInfo* info, MsgHdr* msg); 

    void stopWork();

private:
    virtual Int32 work(); 

    Void addRunQue(FdInfo* info);
    Int32 dealFd(FdInfo* info);

private: 
    list_head m_lock_list;
    
    ManageCenter* m_mng;
    MutexCond* m_cond;
    Bool m_stoped;
};

#endif 

