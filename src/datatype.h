#ifndef __DATATYPE_H__
#define __DATATYPE_H__
#include"globaltype.h"
#include"listnode.h"
#include"msgtype.h"


struct FdInfo;
struct UserConn;
struct EvpBase;
class TickTimer;

struct TcpParam {
    Int32 m_port;
    Int32 m_addr_len;
    Char m_ip[DEF_IP_SIZE];
    Char m_addr[DEF_ADDR_SIZE];
};

struct SockBase { 
    MsgHdr* m_curr_snd;
    MsgHdr* m_curr_rcv;

    Int32 m_hdr_len;
    Byte m_buf[DEF_MSG_HEADER_SIZE];
};

struct EkeyBase {
    Int32 m_status;
    Byte m_seid[DEF_SEID_SIZE];
    Byte m_symm_key[DEF_SYM_KEY_SIZE];
    Byte m_asymm_key[DEF_ASYM_KEY_SIZE];
};

struct NodeBase {
    list_node m_node;
    Int32 m_node_type;
};

struct TcpPairs {
    NodeBase m_base;

    TcpParam m_local;
    TcpParam m_peer;
};

struct EventData {
    NodeBase m_base;

    list_head m_cmd_que;
    FdInfo* m_fdinfo;
    Uint32 m_event_atom;
};

struct TimerData {
    NodeBase m_base;

    FdInfo* m_fdinfo;
    TickTimer* m_timer;
};

struct ListenerTcp {
    NodeBase m_base;

    EkeyBase* m_ekey; // this is aggregation of listener
    FdInfo* m_fdinfo;
    UserConn* m_user; // this is the child of listener
    
    const TcpPairs* m_tcp_pairs;
};

struct ListenerDirty {
    NodeBase m_base;
    
    list_head m_usr_que;

    EkeyBase* m_ekey;
    FdInfo* m_fdinfo;
    
    const TcpPairs* m_tcp_pairs;
};

struct ListenerAdmin {
    NodeBase m_base;
    
    list_head m_admin_que;

    EkeyBase* m_ekey;
    FdInfo* m_fdinfo; 
    
    const TcpPairs* m_tcp_pairs;
};

struct UserAccpt {
    NodeBase m_base; // used by user queue
    
    order_list_head m_session_que; // tcp conn sessions 

    ListenerDirty* m_parent;
    FdInfo* m_fdinfo;

    Uint32 m_user_id;
    Int32 m_usr_status;

    SockBase m_sock;
    EvpBase* m_evp;
};

struct UserConn {
    NodeBase m_base; // used by user queue
    
    order_list_head m_session_que; // tcp accept sessions 

    ListenerTcp* m_parent;
    FdInfo* m_fdinfo;

    Uint32 m_user_id;
    Uint32 m_last_session_id;
    Int32 m_usr_status;

    SockBase m_sock;
    EvpBase* m_evp;
};


struct SessionAccpt {
    NodeBase m_base; // used by session queue

    UserConn* m_parent;
    FdInfo* m_fdinfo;
    
    Uint32 m_session_id;
    Int32 m_status;

    SockBase m_sock;
};

struct SessionConn {
    NodeBase m_base; 

    UserAccpt* m_parent;
    FdInfo* m_fdinfo;
    
    Uint32 m_session_id;
    Int32 m_status;

    SockBase m_sock;
};

struct AdminConn;

struct AdminAccpt {
    NodeBase m_base; 

    ListenerAdmin* m_parent;
    FdInfo* m_fdinfo;
    AdminConn* m_child;
    
    Uint32 m_admin_id;
    Int32 m_admin_status;

    SockBase m_sock;
};

struct AdminConn {
    NodeBase m_base; 
    
    AdminAccpt* m_parent;
    FdInfo* m_fdinfo;
    
    Int32 m_admin_status;

    SockBase m_sock;
};

struct FdInfo { 
    list_node m_io_node; 
    list_node m_deal_node; 

    list_head m_rd_que;
    list_head m_wr_que;
    list_head m_deal_que;
    
    list_head m_send_que;
    list_head m_recv_que;

    Void* m_io_data;
    Void* m_deal_data;
    
    Int32 m_fd;
    Int32 m_rd_type;
    Int32 m_wr_type;
    Int32 m_deal_type;
    Int32 m_fd_status;
    
    Bool m_test_rd; 
    Bool m_test_wr; 
    Bool m_can_wr;
    Bool m_more_wr;
    
    Bool m_wr_err;
    Bool m_rd_err;

    Bool m_is_dealing;
};

enum EnumNodeType {
    ENUM_NODE_TCP_LISTENER,
    ENUM_NODE_DIRTY_LISTENER,
    ENUM_NODE_ADMIN_LISTENER,
    ENUM_NODE_SESS_ACCPT,
    ENUM_NODE_SESS_CONN,
    ENUM_NODE_USER_ACCPT,
    ENUM_NODE_USER_CONN,
    ENUM_NODE_ADMIN_ACCPT,
    ENUM_NODE_ADMIN_CONN,

    ENUM_NODE_EVENT_CMD,
    ENUM_NODE_TIMER,

    ENUM_NODE_END
};

enum EnumRdType {
    ENUM_RD_EVENT_CMD,
    ENUM_RD_TIMER,
    ENUM_RD_TCP_LISTENER,
    ENUM_RD_DIRTY_LISTENER,
    ENUM_RD_ADMIN_LISTENER,
    ENUM_RD_TCP_RAW,
    ENUM_RD_DIRTY_MSG,

    ENUM_RD_END
};

enum EnumWrType {
    ENUM_WR_TCP_CONNECTING,
    ENUM_WR_DIRTY_CONNECTING,
    ENUM_WR_SOCK_MSG,

    ENUM_WR_END
};

enum EnumDealType {
    ENUM_DEAL_SESSION_IN,
    ENUM_DEAL_SESSION_OUT,
    ENUM_DEAL_USER_IN,
    ENUM_DEAL_USER_OUT,

    ENUM_DEAL_ADMIN_IN,
    ENUM_DEAL_ADMIN_OUT,

    ENUM_DEAL_END
};

enum EnumSockStatus {
    ENUM_SOCK_INIT = 0,

    ENUM_SOCK_CONNECTING,
    ENUM_SOCK_ESTABLISH,
    ENUM_SOCK_SHUTDOWN,
    ENUM_SOCK_CLOSING,
    ENUM_SOCK_CLOSED,

    ENUM_SOCK_END
};

enum EnumUserStatus {
    ENUM_USER_INIT,
    ENUM_USER_AUTH_REQ,
    ENUM_USER_EXCH_KEY,
    ENUM_USER_LOGIN,
    ENUM_USER_LOGOUT,

    ENUM_USER_END
};

enum EnumErrCode {
    ENUM_ERR_OK = 0,

    ENUM_ERR_FAIL = 1000,

    ENUM_ERR_CONN_SERVER,
    ENUM_ERR_INVALID_SESS,

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

#endif

