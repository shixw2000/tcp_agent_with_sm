#ifndef __CONFIG_H__
#define __CONFIG_H__
#include<map>
#include<string>
#include"globaltype.h"
#include"listnode.h"
#include"datatype.h"


static const Int32 MAX_LINE_LEN = 128;
static const Int32 MAX_FILENAME_PATH_SIZE = 256;
static const Char DEF_AGENT_CONF_PATH[] = "agent.conf";
static const Char DEF_AGENT_SEC_PRE[] = "agent_info_";
static const Char DEF_AGENT_TYPE_KEY[] = "agent_type";
static const Char DEF_ORIGIN_KEY[] = "origin";
static const Char DEF_ADDRESS_PRE_KEY[] = "bind_addr_";
static const Char DEF_LOCAL_ADDR_PRE_KEY[] = "local_addr_";
static const Char DEF_PEER_ADDR_PRE_KEY[] = "peer_addr_";

static const Char DEF_SEC_GLOBAL_NAME[] = "global";
static const Char DEF_KEY_PASSWD_NAME[] = "password";
static const Char DEF_KEY_LOG_LEVEL_NAME[] = "log_level";


typedef struct Address {
    NodeBase m_base;
    TcpParam m_param;
} Address;

typedef struct AddrPairs {
    NodeBase m_base;
    TcpPairs m_pairs;
} AddrPairs;

typedef struct AgentCli {
    NodeBase m_base;
    
    list_head m_pairs; 
    TcpParam m_origin; 
} AgentCli;

typedef struct AgentSrv {
    NodeBase m_base; 
    
    list_head m_binds; 
} AgentSrv;


struct Config { 
    list_head m_agent_list;
    Int32 m_log_level;
    Char m_passwd[MAX_PIN_PASSWD_SIZE];
};

class Parser {
    typedef std::string typeStr;
    typedef std::map<typeStr, typeStr> typeMap;
    typedef typeMap::iterator typeMapItr;
    typedef std::map<typeStr, typeMap> typeConf;
    typedef typeConf::iterator typeConfItr; 
    
public:
    Parser();
    ~Parser();

    Int32 init(const Char* path);
    Void finish();
    
    inline Config* getConf() {
        return &m_config;
    }

private:
    Int32 parse(const Char* path);
    Int32 parseAddr(TcpParam* param, Char* url);

    Int32 analyseGlobal();
    
    Int32 analyseAgent();

    Int32 analyseAddress(typeMap& imap, list_head* list);
    
    Void strip(Char* text);
    int getKeyVal(const Char* line, Char* key, Int32 key_len,
        Char* val, Int32 val_len);
    Bool getSection(Char line[]);

    Int32 getKeyStr(typeMap& imap, const Char key[],
        Char val[], Int32 maxlen);
    
    Int32 getKeyInt(typeMap& imap, const Char key[], Int32* val);

    Int32 parseAgentCli(Int32 type, typeMap& imap, list_head* list);
    Int32 parseAgentSrv(Int32 type, typeMap& imap, list_head* list);

    Void freeAgents(list_head* list);
    
private:
    Config m_config;
    typeConf m_conf; 
};

#endif

