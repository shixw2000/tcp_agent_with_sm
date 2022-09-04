#ifndef __SOCKOPER_H__
#define __SOCKOPER_H__
#include"globaltype.h"


struct FdInfo;
struct SockBase;
struct MsgHdr;
class ManageCenter;

class SockOper {
public:
    explicit SockOper(ManageCenter* mng);
    ~SockOper();
    
    Int32 readTcpRaw(FdInfo* info, SockBase* sock);
    Int32 readSock(FdInfo* info, SockBase* sock);   
    Int32 writeSock(FdInfo* info, SockBase* sock);

private:
    Bool chkHeader(const MsgHdr* hdr);
    Int32 parse(const Char* buf, int len, FdInfo* info, SockBase* sock); 
    Int32 fill(const Char* buf, int len, FdInfo* info, SockBase* sock);

    Int32 writeMsg(Int32 fd, MsgHdr* msg);

private:
    ManageCenter* m_mng;
};

#endif

