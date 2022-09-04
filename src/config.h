#ifndef __CONFIG_H__
#define __CONFIG_H__
#include<map>
#include<string>
#include"globaltype.h"
#include"listnode.h"


static const Int32 MAX_LINE_LEN = 128;
static const Int32 MAX_FILENAME_PATH_SIZE = 256;
static const Char DEF_AGENT_CONF_PATH[] = "agent.conf";
static const Char DEF_GATEWAY_SEC_PRE[] = "gateway";
static const Char DEF_GATEWAY_TYPE_KEY[] = "gateway_type";
static const Char DEF_LOCAL_IP_KEY[] = "local_ip";
static const Char DEF_LOCAL_PORT_KEY[] = "local_port";
static const Char DEF_PEER_IP_KEY[] = "peer_ip";
static const Char DEF_PEER_PORT_KEY[] = "peer_port";

struct Config { 
    list_head m_tcp_pair_list;
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

    Int32 analyse();
    
    Void strip(Char* text);
    int getKeyVal(const Char* line, Char* key, Int32 key_len,
        Char* val, Int32 val_len);
    Bool getSection(Char line[]);

    Int32 getKeyStr(typeMap& imap, const Char key[],
        Char val[], Int32 maxlen);
    
    Int32 getKeyInt(typeMap& imap, const Char key[], Int32* val);

    Int32 addTcpPairs(Int32 type, const Char szLocalIp[], Int32 localPort, 
        const Char szPeerIp[], Int32 peerPort);

    Void freeTcpPairs(list_head* list);
    
private:
    Config m_config;
    typeConf m_conf; 
};

#endif

