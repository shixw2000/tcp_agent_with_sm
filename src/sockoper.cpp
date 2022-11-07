#include"sockoper.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"listnode.h"
#include"sockutil.h"
#include"interfobj.h"


Char SockOper::g_buf[DEF_TCP_MAX_BUF_SIZE];

Int32 SockOper::readTcpRaw(FdInfo* info, SockBase*, I_Dispatcher* mng) {
    Int32 rdlen = 0;
    MsgHdr* msg = NULL;
    MsgTcpPlain* tcpData = NULL;

    rdlen = recvTcp(info->m_fd, g_buf, DEF_TCP_MAX_BUF_SIZE);
    while (0 < rdlen) {
        msg = MsgCenter::creat<MsgTcpPlain>(ENUM_MSG_CMD_TCP_PLAIN, rdlen);
        msg->m_seq = MsgCenter::seqno();

        tcpData = MsgCenter::cast<MsgTcpPlain>(msg);

        tcpData->m_data_len = rdlen;
        memcpy(tcpData->m_data, g_buf, rdlen);

        MsgCenter::addCrc(msg);

        MsgCenter::printMsg("read_raw_sock_msg", msg);
        
        MsgCenter::reopen(msg);
        
        mng->dispatch(info, msg);
        rdlen = recvTcp(info->m_fd, g_buf, DEF_TCP_MAX_BUF_SIZE);
    }

    if (0 == rdlen) {
        return 0;
    } else {
        return -1;
    }
}

/* return: 0-ok, 1-blocking write, other: error */
Int32 SockOper::writeTcpRaw(FdInfo* info, SockBase* sock) {
    Int32 ret = 0;
    list_node* pos = NULL;

    while (TRUE) {
        if (NULL == sock->m_curr_snd) {
            if (!list_empty(&info->m_wr_que)) {
                pos = LIST_FIRST(&info->m_wr_que);
                list_del(pos, &info->m_wr_que); 
                
                sock->m_curr_snd = MsgCenter::node2msg(pos);

                if (ENUM_MSG_CMD_TCP_PLAIN == sock->m_curr_snd->m_cmd) {
                    /* offset my header before send */
                    MsgCenter::offsetTcpRaw(sock->m_curr_snd);
                } else {
                    /* invalid msg to send */
                    return -2;
                }
            } else {
                /* send all and ok */
                return 0;
            }
        }

        ret = writeMsg(info->m_fd, sock->m_curr_snd);
        if (0 == ret) {
            MsgCenter::printMsg("write_sock_msg", sock->m_curr_snd);
            
            MsgCenter::free(sock->m_curr_snd);
            
            sock->m_curr_snd = NULL;
            continue;
        } else if (1 == ret) {
            /* can not send more */
            return 1;
        } else {
            /* error */
            return -1;
        }
    }
}

Int32 SockOper::readSock(FdInfo* info, SockBase* sock, I_Dispatcher* mng) {
    Int32 rdlen = 0;
    Int32 used = 0;
    Int32 left = 0;
    const Char* psz = NULL;

    rdlen = recvTcp(info->m_fd, g_buf, DEF_TCP_MAX_BUF_SIZE);
    while (0 < rdlen) {
        psz = g_buf;
        left = rdlen;

        while (0 < left) {
            used = parse(psz, left, info, sock, mng);
            if (0 <= used) {
                psz += used;
                left -= used; 
            } else {
                LOG_ERROR("**********read_sock| fd=%d| rdlen=%d| left=%d| used=%d|"
                    " msg=parse error|",
                    info->m_fd, rdlen, left, used);
                return -1;
            }
        }

        rdlen = recvTcp(info->m_fd, g_buf, DEF_TCP_MAX_BUF_SIZE);
    }

    if (0 == rdlen) {
        return 0;
    } else {
        return -1;
    }
}

/* return: 0-ok, 1-blocking write, other: error */
Int32 SockOper::writeSock(FdInfo* info, SockBase* sock) {
    Int32 ret = 0;
    list_node* pos = NULL;

    while (TRUE) {
        if (NULL == sock->m_curr_snd) {
            if (!list_empty(&info->m_wr_que)) {
                pos = LIST_FIRST(&info->m_wr_que);
                list_del(pos, &info->m_wr_que); 
                
                sock->m_curr_snd = MsgCenter::node2msg(pos);
            } else {
                return 0;
            }
        }

        ret = writeMsg(info->m_fd, sock->m_curr_snd);
        if (0 == ret) {
            MsgCenter::printMsg("write_sock_msg", sock->m_curr_snd);
            
            MsgCenter::free(sock->m_curr_snd);
            
            sock->m_curr_snd = NULL;
            continue;
        } else if (1 == ret) {
            /* send blocking */
            return 1;
        } else {
            /* error */
            return -1;
        }
    }
}


/* return: 0: write end, 1: uncompleted, -1: error */
Int32 SockOper::writeMsg(Int32 fd, MsgHdr* msg) {
    Int32 size = 0;
    Int32 used = 0;
    Int32 left = 0;
    Int32 sndlen = 0;
    const Char* psz = NULL; 

    psz = MsgCenter::buffer(msg);
    size = MsgCenter::bufsize(msg);
    used = MsgCenter::bufpos(msg);

    psz += used;
    left = size - used;
    sndlen = sendTcp(fd, psz, left); 
    if (0 <= sndlen) {
        /* forward offset */
        used += sndlen;
        MsgCenter::setbufpos(used, msg); 
            
        if (sndlen == left) {
            return 0;
        } else { 
            LOG_DEBUG("writeMsg| fd=%d| size=%d| used=%d|"
                " left=%d| sndlen=%d| msg=sendTcp|",
                fd, size, used, left, sndlen);
                
            return 1;
        }
    } else {
        LOG_ERROR("****writeMsg| fd=%d| size=%d| used=%d|"
            " left=%d| sndlen=%d| msg=sendTcp error|",
            fd, size, used, left, sndlen);
        
        return -1;
    }
}

Bool SockOper::chkHeader(const MsgHdr* hdr) {
    if (DEF_MSG_HEADER_SIZE <= hdr->m_size 
        && DEF_MSG_MAX_SIZE >= hdr->m_size
        && DEF_MSG_VER == hdr->m_version) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 SockOper::parse(const Char* buf, int len, FdInfo* info, 
    SockBase* sock, I_Dispatcher* mng) {
    Int32 left = 0;
    Int32 used = 0;
    Bool bOk = TRUE;
    const MsgHdr* hdr = NULL;

    if (0 == sock->m_hdr_len && DEF_MSG_HEADER_SIZE <= len) {
        hdr = (const MsgHdr*)buf;
        
        bOk = chkHeader(hdr);
        if (bOk) {
            sock->m_hdr_len = DEF_MSG_HEADER_SIZE;
            sock->m_curr_rcv = MsgCenter::prepend(hdr->m_size);
            if (NULL != sock->m_curr_rcv) {
                used = fill(buf, len, info, sock, mng);
                return used;
            } else {
                return -1;
            }
        } else {
            LOG_ERROR("parse_buffer| hdr_size=0x%x| version=0x%x|"
                " cmd=0x%x| msg=chk header invalid|",
                hdr->m_size, hdr->m_version, hdr->m_cmd);
            return -2;
        }
    } else if (sock->m_hdr_len < DEF_MSG_HEADER_SIZE) {
        left = DEF_MSG_HEADER_SIZE - sock->m_hdr_len;
        if (left <= len) {
            memcpy(&sock->m_buf[sock->m_hdr_len], buf, left);
            
            sock->m_hdr_len = DEF_MSG_HEADER_SIZE;
            buf += left;
            len -= left;

            hdr = (const MsgHdr*)sock->m_buf;
            bOk = chkHeader(hdr);
            if (bOk) {
                sock->m_curr_rcv = MsgCenter::prepend(hdr->m_size);

                /* fill header */
                MsgCenter::fillMsg(sock->m_buf, DEF_MSG_HEADER_SIZE, sock->m_curr_rcv);

                /* file body */
                used = fill(buf, len, info, sock, mng);
                return left + used;
            } else {
                LOG_ERROR("parse_buffer| hdr_size=0x%x| version=0x%x|"
                    " cmd=0x%x| msg=chk header invalid|",
                    hdr->m_size, hdr->m_version, hdr->m_cmd);
                
                return -3;
            }
        } else {
            memcpy(&sock->m_buf[sock->m_hdr_len], buf, len);
            sock->m_hdr_len += len;

            return len;
        }
    } else {
        /* fullfil the msg */
        used = fill(buf, len, info, sock, mng);
        return used;
    }
}

Int32 SockOper::fill(const Char* buf, int len, FdInfo* info, 
    SockBase* sock, I_Dispatcher* mng) {
    Int32 used = 0;
    Bool eom = FALSE;

    used = MsgCenter::fillMsg(buf, len, sock->m_curr_rcv);
    eom = MsgCenter::endOfMsg(sock->m_curr_rcv);
    if (eom) {
        MsgCenter::printMsg("read_sock_msg", sock->m_curr_rcv);
        MsgCenter::reopen(sock->m_curr_rcv);

        mng->dispatch(info, sock->m_curr_rcv);
        
        sock->m_curr_rcv = NULL;
        sock->m_hdr_len = 0; 
    }

    return used;
}

