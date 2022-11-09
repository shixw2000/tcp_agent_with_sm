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

struct MsgNotify {
    Uint64 m_data;
};

/* this is the header format of session msgs */
struct MsgSessHead {
    Uint32 m_user_id;
    Uint32 m_session_id;
};

struct MsgUserHead {
    Uint32 m_user_id;
};

struct MsgChildUserExit {
    Uint32 m_user_id;
    Uint64 m_data;
};

struct MsgStartSess {
    Uint32 m_user_id;
    Uint32 m_session_id;
    Int32 m_port;
    Char m_ip[DEF_IP_SIZE];
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
    Int32 m_secret_len;
    Int32 m_sig_len;
    Byte m_src_seid[DEF_SEID_SIZE];
    Byte m_dst_seid[DEF_SEID_SIZE];
    Byte m_secret[MAX_SECRET_DATA_SIZE];
    Byte m_signature[MAX_SIGNATURE_DATA_SIZE];
};

/* exchange rands */
struct MsgUserExchKey {
    Uint32 m_user_id;
    Int32 m_secret_len;
    Int32 m_sig_len;
    Byte m_src_seid[DEF_SEID_SIZE];
    Byte m_dst_seid[DEF_SEID_SIZE];
    Byte m_secret[MAX_SECRET_DATA_SIZE];
    Byte m_digest[DEF_SM3_DIGEST_SIZE];
    Byte m_signature[MAX_SIGNATURE_DATA_SIZE];
};

struct MsgUserCKeyAck {
    Uint32 m_user_id;
    Int32 m_secret_len;
    Byte m_src_seid[DEF_SEID_SIZE];
    Byte m_dst_seid[DEF_SEID_SIZE];
    Byte m_tmp_pub[DEF_SM2_PUB_KEY_SIZE];
    Byte m_secret[MAX_SECRET_DATA_SIZE];
};

struct MsgUserAuthEnd {
    Uint32 m_user_id;
    Byte m_dst_seid[DEF_SEID_SIZE];
};


#pragma pack(pop) 


enum EnumMsgCmd {
    ENUM_MSG_CMD_NULL = 0,

    ENUM_MSG_SYSTEM_IO_END,
    ENUM_MSG_SYSTEM_CHILD_EXIT,
    ENUM_MSG_SYSTEM_STOP_SESS,
    
    ENUM_MSG_SYSTEM_EXIT = 100,   // system cmds max

    ENUM_MSG_NOTIFY_NEW_SESS,
    ENUM_MSG_NOTIFY_NEW_USER,
    ENUM_MSG_NOTIFY_TICK_TIMER,
    
    ENUM_MSG_CMD_START_SESS,
    ENUM_MSG_USR_ARRIVAL,
    ENUM_MSG_SESS_ARRIVAL,
    ENUM_MSG_USR_SETUP_AUTH,
    
    ENUM_MSG_CMD_TCP_PSEUDO, //unencrypted tcp data for transmission 
    ENUM_MSG_CMD_TCP_PLAIN, 
    ENUM_MSG_CMD_TCP_CIPHER,
    
    ENUM_MSG_USR_AUTH_REQ,
    ENUM_MSG_USR_EXCH_KEY,
    ENUM_MSG_USR_CIPHER_KEY_ACK,
    ENUM_MSG_USR_AUTH_END,
    
    ENUM_MSG_CMD_END
};

static const Int32 DEF_MSG_HEADER_SIZE = sizeof(MsgHdr);
static const Int32 DEF_TCP_DATA_OFFSET = sizeof(MsgHdr) + sizeof(MsgTcpPlain);
static const Int32 DEF_TCP_MAX_BUF_SIZE = 0x100000;
static const Int32 DEF_MSG_MAX_SIZE = 0x200000;


#endif

