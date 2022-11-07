#ifndef __SOCKOPER_H__
#define __SOCKOPER_H__
#include"globaltype.h"
#include"msgtype.h"
#include"datatype.h"


class I_Dispatcher;

class SockOper {
public: 
    static Int32 readTcpRaw(FdInfo* info, SockBase* sock, I_Dispatcher* mng);
    static Int32 writeTcpRaw(FdInfo* info, SockBase* sock);
    
    static Int32 readSock(FdInfo* info, SockBase* sock, I_Dispatcher* mng);   
    static Int32 writeSock(FdInfo* info, SockBase* sock);

private:
    static Bool chkHeader(const MsgHdr* hdr);
    
    static Int32 parse(const Char* buf, int len, FdInfo* info, 
        SockBase* sock, I_Dispatcher* mng); 
    
    static Int32 fill(const Char* buf, int len, FdInfo* info, 
        SockBase* sock, I_Dispatcher* mng);

    static Int32 writeMsg(Int32 fd, MsgHdr* msg);

private:
    static Char g_buf[DEF_TCP_MAX_BUF_SIZE];
};

#endif

