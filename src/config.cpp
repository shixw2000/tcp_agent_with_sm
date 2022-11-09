#include<regex.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"config.h"
#include"datatype.h"
#include"sockutil.h"


Parser::Parser() {
    memset(&m_config, 0, sizeof(m_config));

    INIT_LIST_HEAD(&m_config.m_agent_list);
}

Parser::~Parser() {
}

Int32 Parser::init(const Char* path) {
    Int32 ret = 0;

    if (NULL == path) {
        path = DEF_AGENT_CONF_PATH;
    }
    
    ret = parse(path);
    if (0 != ret) {
        LOG_ERROR("parse_config| ret=%d| path=%s| msg=parse error|",
            ret, path);
        
        return ret;
    }

    ret = analyseGlobal();
    if (0 != ret) {
        LOG_ERROR("analyse_global| ret=%d| path=%s| msg=parse error|",
            ret, path);
        
        return ret;
    }

    ret = analyseAgent();
    if (0 != ret) {
        LOG_ERROR("analyse_listener| ret=%d| path=%s| msg=parse error|",
            ret, path);
        
        return ret;
    }

    return ret;
}

Void Parser::finish() {
    freeAgents(&m_config.m_agent_list);
}

Void Parser::strip(Char* text) {
    Int32 len = 0;
    Char* beg = NULL;
    Char* end = NULL;

    len = strnlen(text, MAX_LINE_LEN-1);
    beg = text;
    end = text + len;

    while (isspace(*beg) && beg < end) {
        ++beg;
    }

    while (beg < end && isspace(*(end-1))) {
        --end;
    }

    if (beg < end) {
        len = end - beg;
        memmove(text, beg, len);
        text[len] = '\0';
    } else {
        text[0] = '\0';
    }
}

int Parser::getKeyVal(const Char* line, Char* key, Int32 key_len,
    Char* val, Int32 val_len) {
    static const Char DEF_STR_PATTERN[] = 
        "^[[:space:]]*([_[:alnum:]]*)[[:space:]]*=[[:space:]]*\"(.*)\"[[:space:]]*$"; 
    int ret = 0;
    int len = 0;
    regex_t reg;
    regmatch_t matchs[3];  
    
    ret = regcomp(&reg, DEF_STR_PATTERN, REG_EXTENDED | REG_NEWLINE);
    if (0 != ret) {
        return -1;
    }

    do {
        ret = regexec(&reg, line, 3, matchs, 0);
        if (0 != ret) { 
            break;
        }
        
        len = (int)(matchs[1].rm_eo-matchs[1].rm_so);
        if (len < key_len) {
            
            memcpy(key, &line[ matchs[1].rm_so ], len);
            key[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        len = (int)(matchs[2].rm_eo-matchs[2].rm_so);
        if (len < val_len) {            
            memcpy(val, &line[ matchs[2].rm_so ], len);
            val[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        ret = 0;
    } while (0);

    regfree(&reg);
    return ret;
}

Bool Parser::getSection(Char line[]) {
    Int32 len = 0;

    len = strnlen(line, MAX_LINE_LEN);
    if (2 < len) {
        if ('[' == line[0] && ']' == line[len-1]) {
            len -= 2;
            memmove(line, &line[1], len);
            line[len] = '\0';
            
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Int32 Parser::parse(const Char* path) {
    Int32 ret = 0;
    Bool bSec = TRUE;
    FILE* hd = NULL;
    Char line[MAX_LINE_LEN] = {0};
    Char key[MAX_LINE_LEN] = {0};
    Char val[MAX_LINE_LEN] = {0};
    Char* psz = NULL;
    typeConfItr itrConf;

    m_conf.clear();

    hd = fopen(path, "rb");
    if (NULL != hd) { 
        itrConf = m_conf.end();
        
        psz = fgets(line, MAX_LINE_LEN, hd);
        while (NULL != psz) {
            strip(line);

            /* ignore empty or comment line */
            if ('#' != line[0] && '\0' != line[0]) {
                bSec = getSection(line);
                if (!bSec) {
                    if (itrConf != m_conf.end()) {
                        ret = getKeyVal(line, key, MAX_LINE_LEN, val, MAX_LINE_LEN);
                        if (0 == ret) {
                            typeMap& imap = itrConf->second;

                            imap[key] = val;
                        } else {
                            /* invalid item */
                            ret = -2;
                            break;
                        }
                    } else {
                        /* outside of a section */
                        ret = -3;
                        break;
                    }
                } else {
                    itrConf = m_conf.find(line);
                    if (itrConf == m_conf.end()) {
                        m_conf[line] = typeMap();
                        itrConf = m_conf.find(line);
                    } else {
                        /* duplicated section */
                        ret = -4;
                        break;
                    }
                }
            }
            
            psz = fgets(line, MAX_LINE_LEN, hd);
        }

        fclose(hd);
    } else {
        ret = -1;
    }

    return ret;
}

Int32 Parser::parseAddr(TcpParam* param, Char* url) {
    Int32 ret = 0;
    Int32 port = 0;
    char* psz = NULL;
    
    psz = strrchr(url, ':');
    if (NULL != psz) {
        *psz++ = '\0';
        port = atoi(psz);

        ret = buildParam(url, port, param);
        return ret;
    } else {
        return -1;
    }
}

Int32 Parser::parseAgentSrv(Int32 type, typeMap& imap, list_head* list) {
    Int32 ret = 0;
    AgentSrv* obj = NULL;
    Address* addr = NULL;
    Char key[MAX_LINE_LEN] = {0};
    Char url[DEF_ADDR_SIZE] = {0};

    I_NEW(AgentSrv, obj);
    
    memset(obj, 0, sizeof(*obj));
    INIT_LIST_NODE(&obj->m_base.m_node);
    INIT_LIST_HEAD(&obj->m_binds); 
    
    obj->m_base.m_node_type = type;

    /* add to list no matter if ok */
    list_add_back(&obj->m_base.m_node, list);

    for (int cnt = 0; ; ++cnt) {
        I_NEW(Address, addr);
        
        memset(addr, 0, sizeof(*addr));
        INIT_LIST_NODE(&addr->m_base.m_node);
        addr->m_base.m_node_type = ENUM_NODE_ADDR;
        
        snprintf(key, MAX_LINE_LEN, "%s%d", DEF_ADDRESS_PRE_KEY, cnt + 1);
        ret = getKeyStr(imap, key, url, DEF_ADDR_SIZE);
        if (0 != ret) {
            if (0 != cnt) {
                /* end of address */
                ret = 0;
            } else {
                ret = -1; 
            }

            break;
        }

        ret = parseAddr(&addr->m_param, url);
        if (0 == ret) {
            /* add a address */
            list_add_back(&addr->m_base.m_node, &obj->m_binds);
            addr = NULL;
        } else {
            ret = -1;
            break;
        }
    }

    I_FREE(addr);
    return ret; 
}

Int32 Parser::parseAgentCli(Int32 type, typeMap& imap, list_head* list) {
    Int32 ret = 0;
    AgentCli* obj = NULL;
    AddrPairs* pairs = NULL;
    Char key[MAX_LINE_LEN] = {0};
    Char url[DEF_ADDR_SIZE] = {0};

    I_NEW(AgentCli, obj);
    
    memset(obj, 0, sizeof(*obj));
    INIT_LIST_NODE(&obj->m_base.m_node);
    INIT_LIST_HEAD(&obj->m_pairs); 
    
    obj->m_base.m_node_type = type;

    /* add to list no matter if ok */
    list_add_back(&obj->m_base.m_node, list);

    ret = getKeyStr(imap, DEF_ORIGIN_KEY, url, DEF_ADDR_SIZE);
    if (0 != ret) {
        return ret;
    }

    ret = parseAddr(&obj->m_origin, url);
    if (0 != ret) {
        return ret;
    }

    for (int cnt = 0; ; ++cnt) {
        I_NEW(AddrPairs, pairs);
        
        memset(pairs, 0, sizeof(*pairs));
        INIT_LIST_NODE(&pairs->m_base.m_node);
        pairs->m_base.m_node_type = ENUM_NODE_ADDR_PAIRS;
        
        snprintf(key, MAX_LINE_LEN, "%s%d", DEF_LOCAL_ADDR_PRE_KEY, cnt + 1);
        ret = getKeyStr(imap, key, url, DEF_ADDR_SIZE);
        if (0 != ret) { 
            if (0 != cnt) {
                /* end of address */
                ret = 0;
            } else {
                ret = -1; 
            }

            break;
        }

        ret = parseAddr(&pairs->m_pairs.m_local, url);
        if (0 != ret) {
            break;
        }

        snprintf(key, MAX_LINE_LEN, "%s%d", DEF_PEER_ADDR_PRE_KEY, cnt + 1);
        ret = getKeyStr(imap, key, url, DEF_ADDR_SIZE);
        if (0 != ret) { 
            break;
        }

        ret = parseAddr(&pairs->m_pairs.m_peer, url);
        if (0 != ret) {
            break;
        }
        
        list_add_back(&pairs->m_base.m_node, &obj->m_pairs);
    }

    I_FREE(pairs);
    return ret; 
}

Int32 Parser::analyseGlobal() {
    Int32 ret = 0;
    typeConfItr itrConf;
    typeMapItr itrKey;

    itrConf = m_conf.find(DEF_SEC_GLOBAL_NAME);
    if (m_conf.end() == itrConf) {
        return -1;
    }

    do {
        typeMap& imap = itrConf->second;
        
        ret = getKeyStr(imap, DEF_KEY_PASSWD_NAME, m_config.m_passwd, 
            MAX_PIN_PASSWD_SIZE);
        if (0 != ret) {
            LOG_ERROR("analyse_global| msg=invalid password|");
            break;
        } 
    } while (0);

    return ret;
}

Int32 Parser::analyseAgent() {
    Int32 ret = 0;
    Int32 type = 0;
    typeConfItr itrConf;
    typeMapItr itrKey;
    Char sec[MAX_LINE_LEN] = {0};
    
    for (int cnt = 0; ; ++cnt) {
        snprintf(sec, MAX_LINE_LEN, "%s%d", DEF_AGENT_SEC_PRE, cnt+1);
        itrConf = m_conf.find(sec);
        if (m_conf.end() == itrConf) {
            if (0 < cnt) {
                /* end of agent */
                ret = 0;
            } else {
                ret = -1;
            }
            
            break;
        }

        typeMap& imap = itrConf->second; 
        
        ret = getKeyInt(imap, DEF_AGENT_TYPE_KEY, &type);
        if (0 != ret) {
            break;
        }

        if (ENUM_NODE_SESS_LISTENER == type
            || ENUM_NODE_SESS_LISTENER_PSEUDO == type) {
            ret = parseAgentCli(type, imap, &m_config.m_agent_list);
        } else if (ENUM_NODE_USR_LISTENER == type
            || ENUM_NODE_USR_LISTENER_PSEUDO == type) {
            ret = parseAgentSrv(type, imap, &m_config.m_agent_list);
        } else {
            ret = -1;
        }

        if (0 != ret) {
            break;
        } 
    } 

    return ret;
}

Int32 Parser::getKeyStr(typeMap& imap, const Char key[],
    Char val[], Int32 maxlen) {
    Int32 size = 0;
    typeMapItr itrKey;

    itrKey = imap.find(key);
    if (imap.end() != itrKey) {
        const typeStr& str = itrKey->second;

        size = (Int32)str.size();
        if (size < maxlen) {
            str.copy(val, size);
            val[size] = '\0';

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Int32 Parser::getKeyInt(typeMap& imap, const Char key[], Int32* val) {
    Int32 ret = 0;
    Char buf[MAX_LINE_LEN] = {0};

    ret = getKeyStr(imap, key, buf, MAX_LINE_LEN);
    if (0 == ret) {
        if (isdigit(buf[0])) {
            *val = atoi(buf);
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Void Parser::freeAgents(list_head* list) {
    list_node* pos1 = NULL;
    list_node* n1 = NULL;
    list_node* pos2 = NULL;
    list_node* n2 = NULL;
    NodeBase* base = NULL;
    AgentCli* cli = NULL;
    AgentSrv* srv = NULL;
    Address* addr = NULL;
    AddrPairs* pairs = NULL;
    
    list_for_each_safe(pos1, n1, list) {
        list_del(pos1, list);

        base = (NodeBase*)pos1;

        if (ENUM_NODE_SESS_LISTENER == base->m_node_type
            || ENUM_NODE_SESS_LISTENER_PSEUDO == base->m_node_type) {
            cli = (AgentCli*)pos1;
            
            list_for_each_safe(pos2, n2, &cli->m_pairs) {
                list_del(pos2, list);

                pairs = (AddrPairs*)pos2;
                I_FREE(pairs);
            }

            I_FREE(cli);
        } else if (ENUM_NODE_USR_LISTENER == base->m_node_type
            || ENUM_NODE_USR_LISTENER_PSEUDO == base->m_node_type) {
            srv = (AgentSrv*)pos1;
            
            list_for_each_safe(pos2, n2, &srv->m_binds) {
                list_del(pos2, list);

                addr = (Address*)pos2;
                I_FREE(addr);
            }

            I_FREE(srv);
        } else {
        } 
    }
}

