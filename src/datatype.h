#ifndef __DATATYPE_H__
#define __DATATYPE_H__
#include"globaltype.h"
#include"listnode.h"
#include"msgtype.h"


struct FdInfo;
struct UserConn;
struct EvpBase;
class TickTimer;

struct TimerEle {
    hlist_node m_node;
    TickTimer* m_base;
    Uint32 m_expires;
    Uint32 m_interval; 
    Int32 m_type;
};

struct TcpParam {
    Int32 m_port;
    Int32 m_addr_len;
    Char m_ip[DEF_IP_SIZE];
    Char m_addr[DEF_ADDR_SIZE];
};

struct TcpPairs {
    TcpParam m_local;
    TcpParam m_peer;
};

struct SockBase { 
    MsgHdr* m_curr_snd;
    MsgHdr* m_curr_rcv;

    Int32 m_hdr_len;
    Byte m_buf[DEF_MSG_HEADER_SIZE];
};

struct SysBase {
    Byte m_seid[MAX_SEID_SIZE];
    Byte m_rand[DEF_RAND_CODE_SIZE];
};

struct ExtraData {
    Int32 m_sess_key_size;
    Byte m_sess_key[MAX_SYM_KEY_SIZE];
    Byte m_peer_seid[MAX_SEID_SIZE];    
    Byte m_rand1[DEF_RAND_CODE_SIZE];
    Byte m_rand2[DEF_RAND_CODE_SIZE];
};

struct NodeBase {
    list_node m_node;
    Int32 m_node_type;
};

struct ListenerTcp {
    NodeBase m_base;

    FdInfo* m_fdinfo;
    UserConn* m_user; // this is the child of listener
    
    const TcpPairs* m_pairs; 
};

struct ListenerDirty {
    NodeBase m_base;
    
    list_head m_usr_que;

    FdInfo* m_fdinfo;
    
    const TcpParam* m_param;
};

struct UserAccpt {
    NodeBase m_base; // used by user queue
    
    order_list_head m_session_que; // tcp conn sessions 

    ListenerDirty* m_parent;
    FdInfo* m_fdinfo;
    const TcpParam* m_param;

    Uint32 m_user_id;
    Int32 m_usr_status;

    SockBase m_sock;
    ExtraData m_extra_data;
    EvpBase* m_evp_rcv;
    EvpBase* m_evp_snd;
};

struct UserConn {
    NodeBase m_base; // used by user queue
    
    list_head m_listener_que;
    order_list_head m_session_que; // tcp accept sessions 

    FdInfo* m_fdinfo; 
    const TcpParam* m_param;
    
    Uint32 m_user_id;
    Int32 m_usr_status;
    Bool m_connected;
    Bool m_peer_open;

    SockBase m_sock;
    ExtraData m_extra_data;
    EvpBase* m_evp_rcv;
    EvpBase* m_evp_snd;
};


struct SessionAccpt {
    NodeBase m_base; // used by session queue

    UserConn* m_parent;
    FdInfo* m_fdinfo;
    const TcpPairs* m_pairs;
    
    Uint32 m_session_id;
    Bool m_peer_open;

    SockBase m_sock;
};

struct SessionConn {
    NodeBase m_base; 

    UserAccpt* m_parent;
    FdInfo* m_fdinfo;
    
    Uint32 m_session_id;
    Bool m_peer_open;
    Bool m_connected;

    TcpParam m_param;
    SockBase m_sock;
};


struct ListenerRouter {
    NodeBase m_base;
    
    list_head m_route_pair_que;

    FdInfo* m_fdinfo;

    Int32 m_enc_type; // check if encrypt or decrypt 
    
    const TcpPairs* m_tcp_pairs;
};


struct Router {
    FdInfo* m_fdinfo;
    
    Int32 m_usr_status;
    Bool m_peer_open;

    SockBase m_sock;
    ExtraData m_extra_data;
    EvpBase* m_evp_rcv;
    EvpBase* m_evp_snd;
};

struct RouterPair {
    NodeBase m_base;

    ListenerRouter* m_parent;

    Uint32 m_router_id;
    Bool m_connected;
    
    Router m_router_in;
    Router m_router_out;
};


struct FdInfo { 
    struct Task m_wr_task;
    struct Task m_mng_task;
    struct Task m_deal_task; 

    struct TimerEle m_heartbeat_timer;

    list_node m_run_node;
    list_node m_rd_node;
    list_node m_flash_node;

    list_head m_rd_que;
    list_head m_wr_que;
    list_head m_deal_que;
    
    list_head m_send_que;
    list_head m_recv_que;

    NodeBase* m_data;
    
    Int32 m_fd;
    Int32 m_fd_type;
    Uint32 m_last_time;
    
    Bool m_test_rd; 
    Bool m_rd_err;
    Bool m_test_wr; 
    Bool m_can_wr;
    Bool m_wr_err;
    
    Bool m_io_run;
    Bool m_deal_err; 
    Bool m_closing;
};

enum EnumNodeType {
    ENUM_NODE_OBJ_MIN = 0,
    ENUM_NODE_SESS_LISTENER = ENUM_NODE_OBJ_MIN,    // min obj used 
    ENUM_NODE_USR_LISTENER,

    ENUM_NODE_SESS_LISTENER_PSEUDO,
    ENUM_NODE_USR_LISTENER_PSEUDO, 
        
    ENUM_NODE_SOCK_DATA_MIN,        // sock data min
    
    ENUM_NODE_SESS_ACCPT,
    ENUM_NODE_SESS_CONN,
    ENUM_NODE_SESS_ACCPT_PSEUDO,
    ENUM_NODE_SESS_CONN_PSEUDO,

    ENUM_NODE_SOCK_FLASH_MIN,   // sock min type
    
    ENUM_NODE_USR_ACCPT,
    ENUM_NODE_USR_CONN, 
    ENUM_NODE_USR_ACCPT_PSEUDO,

    ENUM_NODE_SOCK_FLASH_MAX, // sock max type
    ENUM_NODE_SOCK_DATA_MAX,    // sock data max
    
    ENUM_NODE_EVENT,
    ENUM_NODE_TIMER, 

    ENUM_NODE_OBJ_MAX,      // max obj 

    ENUM_NODE_FACTORY,
    ENUM_NODE_ADDR,
    ENUM_NODE_ADDR_PAIRS,

    ENUM_NODE_END
};


enum EnumUserStatus {
    ENUM_USER_INIT,
    ENUM_USER_WAIT_REQ,
    ENUM_USER_AUTH_REQ,
    ENUM_USER_EXCH_KEY,
    ENUM_USER_EXCH_KEY_ACK,
    ENUM_USER_LOGIN,
    
    ENUM_USER_LOGOUT,

    ENUM_USER_ERROR,

    ENUM_USER_END
};

enum EnumErrCode {
    ENUM_ERR_OK = 0,

    ENUM_ERR_FAIL = 1000,

    ENUM_ERR_CONN_SERVER,
    ENUM_ERR_SESS_ALREADY_EXISTS,
    ENUM_ERR_CONN_SESSEION,
    ENUM_ERR_INVALID_SESS,
    ENUM_ERR_DECRYPT_TXT,
    ENUM_ERR_ENCRYPT_TXT,
    ENUM_ERR_NOT_LOGIN_GATEWAY,
    ENUM_ERR_NOT_LOGIN_USER,
    ENUM_ERR_AUTH_GATEWAY,
    ENUM_ERR_AUTH_STATUS_INVALID,
    ENUM_ERR_PARSE_AUTH_REQ,
    ENUM_ERR_OPER_EKEY,
    ENUM_ERR_CREAT_EXCHG_KEY,
    ENUM_ERR_START_CONN_GATEWAY,
    ENUM_ERR_GATEWAY_STOP,

    ENUM_ERR_PEER_NOT_READY,
    ENUM_ERR_SEND_MSG,
    ENUM_ERR_DISPATCH_MSG,

    ENUM_ERR_END
};

enum EnumDigestAlgo {
    ENUM_DIGEST_SM3,
};

enum EnumCipherAlgo {
    ENUM_CIPHER_SM4,
};

enum EnumPkeyAlgo {
    ENUM_PKEY_SM2,
};

enum EnumTimerType {
    ENUM_TIMER_TYPE_MINUTELY,
    ENUM_TIMER_TYPE_HOURLY,

    ENUM_TIMER_CHK_FLASH,
    ENUM_TIMER_HEAR_BEAT,
    ENUM_TIMER_CHK_LOGIN,

    ENUM_TIMER_TYPE_END
};

static const Uint32 DEF_MINUTE_TICK_CNT = 60;
static const Uint32 DEF_HOUR_TICK_CNT = DEF_MINUTE_TICK_CNT * 60;

/* sock flash timeout */
static const Uint32 DEF_CHK_FLASH_INTERVAL = 10;
static const Uint32 DEF_HEART_BEAT_INTERVAL = 30;
static const Uint32 MAX_FLASH_TIMEOUT_TICK = DEF_HEART_BEAT_INTERVAL * 6; 
static const Uint32 DEF_CHK_LOGIN_INTERVAL = DEF_MINUTE_TICK_CNT;

#endif

