#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__
#include"globaltype.h"
#include"listnode.h"
#include"msgtype.h"


class MsgCenter { 
    struct _internal;
    
public: 

    template<typename T>
    static MsgHdr* creat(Int16 cmd, Int32 extlen = 0) {
        MsgHdr* hdr = NULL;
        Int32 len = sizeof(MsgHdr) + sizeof(T) + extlen;

        hdr = prepend(len); 

        hdr->m_version = DEF_MSG_VER;
        hdr->m_cmd = cmd;

        setsize(len, hdr);
        
        return hdr;
    }
    
    template<typename T>
    static Void shrink(Int32 extlen, MsgHdr* msg) {
        Int32 len = sizeof(MsgHdr) + sizeof(T) + extlen;

        setsize(len, msg);
    }

    static Void setsize(Int32 size, MsgHdr* msg);

    static Uint32 seqno();
    static Uint32 calcCrc(const Void* data, Int32 len);
    static Int32 addCrc(MsgHdr* msg);
    static Int32 chkCrc(MsgHdr* msg);
    
    static MsgHdr* prepend(Int32 len);
    
    static Void free(MsgHdr* msg);

    static MsgHdr* node2msg(list_node* node);

    template<typename T>
    static T* cast(MsgHdr* msg) {
        Void* body = to(msg);
        return (T*)body;
    }

    static MsgHdr* from(Void* body);

    static void notify(MsgHdr* msg, list_head*);
    static void emerge(MsgHdr* msg, list_head*);

    static void add(MsgHdr* msg, order_list_head* head);

    static Int32 fillMsg(const Void* buf, Int32 len, MsgHdr* msg);

    static Bool endOfMsg(MsgHdr* msg);
    
    static Void reopen(MsgHdr* msg);
    static Void offsetTcpRaw(MsgHdr* msg);

    static Char* buffer(MsgHdr* msg);
    static Int32 bufsize(MsgHdr* msg); 
    static Int32 bufpos(MsgHdr* msg);
    static Void setbufpos(Int32 pos, MsgHdr* msg);

    static Void printMsg(const Char* prompt, MsgHdr* msg);

private: 
    static list_node* msg2node(MsgHdr* msg);

    static _internal* msg2Intern(MsgHdr* msg); 

    static Void* to(MsgHdr* msg); 
};

#endif

