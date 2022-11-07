#ifndef __SOCKPOT_H__
#define __SOCKPOT_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


struct MsgHdr;
class SockMng;
class SockCenter;

class SockEvent : public I_FdObj {
public:
    SockEvent(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

private:
    Int32 procEnd(EventData* ev, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

#endif

