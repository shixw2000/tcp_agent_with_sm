#ifndef __MSGTYPE_H__
#define __MSGTYPE_H__
#include"globaltype.h"


#pragma pack(push, 1)

struct MsgHdr {
    Int32 m_size;
    Uint32 m_seq;
    Uint32 m_crc;
    Uint32 m_retcode;
    Uint16 m_version;
    Uint16 m_cmd;
};

struct MsgStartPeer {
    Uint32 m_user_id;
    Uint32 m_session_id;
};

struct MsgStopPeer {
    Uint32 m_user_id;
    Uint32 m_session_id;
    Int32 m_reason;
};

struct MsgCloseSock {
    Int32 m_reason;
};

struct MsgCloseChild {
    Uint32 m_user_id;
    Uint32 m_session_id;
    Int32 m_reason;
};

struct MsgStopGateway {
    Uint32 m_route_id;
    Int32 m_reason;
};

struct MsgTcpPlain {
    Uint32 m_user_id;
    Uint32 m_session_id;
    Int32 m_data_len;
    Byte m_data[0];
};

struct MsgTcpCipher {
    Uint32 m_user_id;
    Uint32 m_session_id;
    Uint32 m_digest_type:8,
        m_cipher_type:8,
        m_pkey_type:8,
        m_res:8;
    Int32 m_plain_len;
    Int32 m_cipher_len;
    Uint32 m_mac;
    Byte m_data[0];
};

/* send the signature of the user */
struct MsgUserAuthReq {
    Uint32 m_user_id;
    Char m_src_seid[DEF_SEID_SIZE];
    Char m_dst_seid[DEF_SEID_SIZE];
    Byte m_rand[DEF_RAND_CODE_SIZE];
    Byte m_secret[DEF_SECRET_DATA_SIZE];
    Byte m_signature[DEF_SIGNATURE_DATA_SIZE];
};

/* exchange rands */
struct MsgUserExchKey {
    Uint32 m_user_id;
    Char m_src_seid[DEF_SEID_SIZE];
    Char m_dst_seid[DEF_SEID_SIZE];
    Byte m_rand[DEF_RAND_CODE_SIZE];
    Byte m_secret[DEF_SECRET_DATA_SIZE];
    Byte m_signature[DEF_SIGNATURE_DATA_SIZE];
};

struct MsgUserCKeyAck {
    Uint32 m_user_id;
    Char m_src_seid[DEF_SEID_SIZE];
    Char m_dst_seid[DEF_SEID_SIZE];
    Byte m_rand[DEF_RAND_CODE_SIZE];
    Byte m_secret[DEF_SECRET_DATA_SIZE];
};

struct MsgUserAuthEnd {
    Uint32 m_user_id;
    Char m_src_seid[DEF_SEID_SIZE];
    Char m_dst_seid[DEF_SEID_SIZE];
};

struct MsgSessArrival {
    Uint32 m_user_id;
    Uint32 m_session_id;
};

struct MsgUserArrival {
    Uint32 m_user_id;
};

struct MsgAdmEkeyInit {
};

struct MsgAdmEkeyLogin {
};

struct MsgAdmEkeyLogout {
};

struct MsgTimerTick {
    Uint32 m_cnt;
};

struct MsgTimerTimeout {
    Int32 m_type;
};

#pragma pack(pop) 


enum EnumMsgCmd {
    ENUM_MSG_CMD_NULL = 0,

    ENUM_MSG_CMD_START_PEER,
    ENUM_MSG_CMD_STOP_PEER,
    ENUM_MSG_CMD_CLOSE_SOCK,
    ENUM_MSG_CMD_CLOSE_CHILD,

    ENUM_MSG_CMD_STOP_GATEWAY,
    
    ENUM_MSG_CMD_TCP_PLAIN,
    ENUM_MSG_CMD_TCP_CIPHER,

    ENUM_MSG_USR_AUTH_REQ,
    ENUM_MSG_USR_EXCH_KEY,
    ENUM_MSG_USR_CIPHER_KEY_ACK,
    ENUM_MSG_USR_AUTH_END,

    ENUM_MSG_USR_ARRIVAL,
    ENUM_MSG_SESS_ARRIVAL,

    ENUM_MSG_CMD_TIMER_TICK,
    ENUM_MSG_CMD_TIMEOUT,

    ENUM_MSG_CMD_ADM_INIT_EKEY,
    ENUM_MSG_CMD_AUTH_PKEY_REQ,
    ENUM_MSG_CMD_AUTH_PKEY_ACK,

    /* local cmds */
    ENUM_LOCAL_CMD_ADD_FD,
    ENUM_LOCAL_CMD_DEL_FD,
    
    ENUM_MSG_CMD_END
};

static const Int32 DEF_MSG_HEADER_SIZE = sizeof(MsgHdr);
static const Int32 DEF_TCP_DATA_OFFSET = sizeof(MsgHdr) + sizeof(MsgTcpPlain);
static const Int32 DEF_TCP_MAX_BUF_SIZE = 0x100000;
static const Int32 DEF_MSG_MAX_SIZE = 0x200000;


#endif

