#ifndef __SMUTIL_H__
#define __SMUTIL_H__
#include"globaltype.h"


class Sm4Util { 
    static const Int32 MAX_SM4_CTX_SIZE = 256;
    
public:
    Sm4Util();
    ~Sm4Util();

    Void setUsrKey(const Void* usr_key, Int32 len);

    Void setIvKey(const Void* iv, const Void* key);
    
    /* enc with padding */
    Int32 sm4_cbc_encrypt(const Void* data, Int32 len, Void* out) const;
    Int32 sm4_cbc_decrypt(const Void* data, Int32 len, Void* out) const;

    Int32 sm4_ecb_encrypt(const Void* data, Int32 len, Void* out) const;
    Int32 sm4_ecb_decrypt(const Void* data, Int32 len, Void* out) const;

private:
    Int32 trunc(const Byte* data, Int32 len, Byte* final) const;
    
private:
    Byte m_iv[DEF_SM4_BLOCK_SIZE];
    Byte m_key[MAX_SM4_CTX_SIZE];
};

class Sm3Util {
    static const Int32 MAX_DIGEST_CTX_SIZE = 256;
    
public:
    Sm3Util();
    ~Sm3Util();
    
    static Int32 digest(const Void* data, Int32 len, Void* output);

    Void init();
    Void update(const Void* data, Int32 len);
    Int32 finish(Void* output);
    
private:
    Byte m_ctx[MAX_DIGEST_CTX_SIZE];
};

class Sm2Util {
};

struct EvpBase {
    Uint32 m_digest_type:8,
        m_cipher_type:8,
        m_pkey_type:8,
        m_padding:8; 
    
    Sm2Util m_pkey;
    Sm3Util m_digest;
    Sm4Util m_cipher; 
};


#endif

