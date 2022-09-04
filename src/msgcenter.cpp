#include"msgcenter.h"
#include"sockutil.h"


struct MsgCenter::_internal {
    list_node m_node;
    Int32 m_capacity;
    Int32 m_size;
    Int32 m_pos;
};

Uint32 MsgCenter::calcCrc(const Void* data, Int32 len) {
    Int32 n = 0;
    Int32 left = 0;
    Uint32 crc = 0;
    Uint32 buf[4] = {0};
    const Uint32* pu = (const Uint32*)data;
    const Byte* psz = (const Byte*)data;

    for (n=0; n+4<=len; n+=4) {
        crc += *(pu++);
    }

    if (n < len) {
        left = len - n;
        memcpy(buf, &psz[n], left);
        memset(&buf[left], 0, 4-left);

        pu = (const Uint32*)buf;
        crc += *pu;
    }

    return ~crc;
}

Int32 MsgCenter::addCrc(MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg); 

    if (msg->m_size == intern->m_size) {
        msg->m_crc = calcCrc(msg, msg->m_size);

        LOG_DEBUG("++++add_crc| _capacity=%d| _size=%d|"
            " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
            " hdr_version=0x%x| hdr_cmd=0x%x|"
            " msg=ok|",
            intern->m_capacity, intern->m_size,
            msg->m_size, msg->m_seq, msg->m_crc, 
            msg->m_version, msg->m_cmd);

        return 0;
    } else {
        LOG_ERROR("****add_crc| _capacity=%d| _size=%d|"
            " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
            " hdr_version=0x%x| hdr_cmd=0x%x|"
            " msg=invalid msg size|",
            intern->m_capacity, intern->m_size,
            msg->m_size, msg->m_seq, msg->m_crc, 
            msg->m_version, msg->m_cmd);

        return -1;
    }
}

Int32 MsgCenter::chkCrc(MsgHdr* msg) {
    Uint32 crc = 0;
    _internal* intern = NULL;

    intern = msg2Intern(msg); 

    if (msg->m_size == intern->m_size) {
        crc = calcCrc(msg, msg->m_size);
        if (!crc) {
            LOG_DEBUG("++++chk_crc| _capacity=%d| _size=%d|"
                " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
                " hdr_version=0x%x| hdr_cmd=0x%x| calc_crc=0x%x|"
                " msg=chk crc ok|",
                intern->m_capacity, intern->m_size,
                msg->m_size, msg->m_seq, msg->m_crc, 
                msg->m_version, msg->m_cmd, crc);
            
            return 0;
        } else {
            LOG_ERROR("****chk_crc| _capacity=%d| _size=%d|"
                " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
                " hdr_version=0x%x| hdr_cmd=0x%x| calc_crc=0x%x|"
                " msg=chk crc invalid|",
                intern->m_capacity, intern->m_size,
                msg->m_size, msg->m_seq, msg->m_crc, 
                msg->m_version, msg->m_cmd, crc);
                
            return -1;
        }

        return 0;
    } else {
        LOG_ERROR("****chk_crc| _capacity=%d| _size=%d|"
            " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
            " hdr_version=0x%x| hdr_cmd=0x%x|"
            " msg=chk size invalid|",
            intern->m_capacity, intern->m_size,
            msg->m_size, msg->m_seq, msg->m_crc, 
            msg->m_version, msg->m_cmd);

        return -1;
    } 
}

void MsgCenter::notify(MsgHdr* msg, list_head* head) {
    _internal* intern = NULL;

    intern = msg2Intern(msg); 
    list_add_back(&intern->m_node, head); 
}

void MsgCenter::emerge(MsgHdr* msg, list_head* head) {
    _internal* intern = NULL;

    intern = msg2Intern(msg); 
    list_add_front(&intern->m_node, head);
}

void MsgCenter::add(MsgHdr* msg, order_list_head* head) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    order_list_add(&intern->m_node, head); 
}

Uint32 MsgCenter::seqno() {
    static Uint32 g_seq = sysRand();
    Uint32 n = 0;

    n = ATOMIC_INC_FETCH(&g_seq);
    return n;
}

MsgHdr* MsgCenter::prepend(Int32 len) { 
    _internal* intern = NULL;
    MsgHdr* hdr = NULL;
    Char* buf = NULL;
    Int32 total = 0;

    total = (Int32)sizeof(_internal) + len;
    ARR_NEW(Char, total, buf);
    
    if (NULL != buf) { 
        memset(buf, 0, total);
        
        intern = (_internal*)buf; 
        hdr = (MsgHdr*)(intern + 1);
        
        INIT_LIST_NODE(&intern->m_node);
        intern->m_capacity = len;
        intern->m_size = len;
        intern->m_pos = 0;

        return hdr;
    } else {
        return NULL;
    }
}

Void MsgCenter::setsize(Int32 size, MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);

    if (size <= intern->m_capacity) {
        intern->m_size = size;
        msg->m_size = size;
    }
}

void MsgCenter::free(MsgHdr* msg) { 
    if (NULL != msg) {
        Char* buf = (Char*)msg;
        
        buf -= sizeof(_internal); 
        ARR_FREE(buf);
    }
}

list_node* MsgCenter::msg2node(MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg); 
    return &intern->m_node;
}

MsgHdr* MsgCenter::node2msg(list_node* node) {
    _internal* intern = NULL;
    
    intern = list_entry(node, _internal, m_node);
    
    return reinterpret_cast<MsgHdr*>(intern+1);
}

MsgCenter::_internal* MsgCenter::msg2Intern(MsgHdr* msg) {
    Char* buf = (Char*)msg;
    _internal* intern = NULL;

    buf -= sizeof(_internal); 
    intern = (_internal*)buf;

    return intern;
}

Void* MsgCenter::to(MsgHdr* msg) {
    return msg + 1;
}

MsgHdr* MsgCenter::from(Void* body) {
    Char* buf = (Char*)body;

    buf -= sizeof(MsgHdr);
    return reinterpret_cast<MsgHdr*>(buf);
}

Int32 MsgCenter::fillMsg(const Void* buf, Int32 len, MsgHdr* msg) {
    Int32 left = 0;
    Char* psz = NULL;
    _internal* intern = NULL;

    psz = (Char*)msg;
    intern = msg2Intern(msg);
    
    psz += intern->m_pos; 
    left = intern->m_size - intern->m_pos;
    if (left <= len) {
        /* read all */
        memcpy(psz, buf, left);
        
        intern->m_pos = intern->m_size;
        return left;
    } else {
        /* read partial */
        memcpy(psz, buf, len);
        intern->m_pos += len;

        return len;
    } 
}

Bool MsgCenter::endOfMsg(MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    return intern->m_size == intern->m_pos;
}

Char* MsgCenter::buffer(MsgHdr* msg) {
    return (Char*)msg;
}

Int32 MsgCenter::bufsize(MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    return intern->m_size;
}

Int32 MsgCenter::bufpos(MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    return intern->m_pos;
}

Void MsgCenter::reopen(MsgHdr* msg) {
    setbufpos(0, msg);
}

Void MsgCenter::offsetTcpRaw(MsgHdr* msg) {
    MsgCenter::setbufpos(DEF_TCP_DATA_OFFSET, msg); 
}

Void MsgCenter::setbufpos(Int32 pos, MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    intern->m_pos = pos;
}

Void MsgCenter::printMsg(const Char* prompt, MsgHdr* msg) {
    _internal* intern = NULL;

    intern = msg2Intern(msg);
    
    LOG_DEBUG("<%s>_print_msg| _capacity=%d| _size=%d| pos=%d|"
        " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
        " hdr_version=0x%x| hdr_cmd=0x%x|",
        prompt,
        intern->m_capacity, intern->m_size, intern->m_pos,
        msg->m_size, msg->m_seq, msg->m_crc, 
        msg->m_version, msg->m_cmd);
}

