#include"smutil.h"
#include"sm_extra.h"


Sm3Util::Sm3Util() {
}

Sm3Util::~Sm3Util() {
}

Int32 Sm3Util::digest(const Void* data, Int32 len, Void* output) {
    Byte* out = (Byte*)output;
    SM3_CTX ctx;

    if (NULL != output) {
        sm3_init(&ctx);
        sm3_update(&ctx, data, len);
        sm3_final(out, &ctx);
    }

    return SM3_DIGEST_LENGTH;
}

Void Sm3Util::init() { 
    SM3_CTX* ctx = (SM3_CTX*)m_ctx;

    sm3_init(ctx);
}

Void Sm3Util::update(const Void* data, Int32 len) {
    SM3_CTX* ctx = (SM3_CTX*)m_ctx;
    
    sm3_update(ctx, data, len);
}

Int32 Sm3Util::finish(Void* output) {
    SM3_CTX* ctx = (SM3_CTX*)m_ctx;
    Byte* out = (Byte*)output;
    
    sm3_final(out, ctx);

    return SM3_DIGEST_LENGTH;
}


Sm4Util::Sm4Util() {
}

Sm4Util::~Sm4Util() {
}

Void Sm4Util::setUsrKey(const Void* usr_key, Int32 len) {
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};
    
    Sm3Util::digest(usr_key, len, digest);
    memcpy(m_key, digest, DEF_SM4_BLOCK_SIZE);
    memcpy(m_iv, &digest[DEF_SM4_BLOCK_SIZE], DEF_SM4_BLOCK_SIZE);
}

Void Sm4Util::setIvKey(const Void* iv, const Void* key) {
    memcpy(m_key, key, DEF_SM4_BLOCK_SIZE);
    memcpy(m_iv, iv, DEF_SM4_BLOCK_SIZE);
}

Int32 Sm4Util::trunc(const Byte* data, Int32 len, Byte* final) const {
    Int32 trunc_len = 0;
    Int32 left = 0;
    Byte n = 0;
    
    trunc_len = len & (~0xF);
    if (trunc_len < len) {
        left = len - trunc_len;
        n = (Byte)(DEF_SM4_BLOCK_SIZE - left);
        
        memcpy(final, &data[trunc_len], left);
        memset(&final[left], n, n);
    } else {
        n = (Byte)DEF_SM4_BLOCK_SIZE;
        
        memset(final, n, n);
    }
    
    return trunc_len;
}

Int32 Sm4Util::sm4_cbc_encrypt(const Void* data, 
    Int32 len, Void* out) const {
    const Byte* input = (const Byte*)data;
    Byte* output = (Byte*)out;
    Int32 outlen = 0;
    Int32 trunc_len = 0;
    SM4_KEY stKey;
    Byte final[DEF_SM4_BLOCK_SIZE] = {0};
    Byte ivec[DEF_SM4_BLOCK_SIZE] = {0}; 

    /* padding */
    outlen = (len + DEF_SM4_BLOCK_SIZE) & (~0xF);
    if (NULL == out) {
        return outlen;
    }
    
    trunc_len = trunc(input, len, final);
    
    SM4_set_key(m_key, &stKey);
    memcpy(ivec, m_iv, DEF_SM4_BLOCK_SIZE);
    
    CRYPTO_cbc128_encrypt(input, output, trunc_len, &stKey, 
        ivec, (block128_f)SM4_encrypt);

    CRYPTO_cbc128_encrypt(final, &output[trunc_len], DEF_SM4_BLOCK_SIZE,
        &stKey, ivec, (block128_f)SM4_encrypt);

    return outlen;
}

Int32 Sm4Util::sm4_cbc_decrypt(const Void* data, 
    Int32 len, Void* out) const { 
    const Byte* input = (const Byte*)data;
    Byte* output = (Byte*)out;
    Int32 outlen = len;
    SM4_KEY stKey;
    Byte ivec[DEF_SM4_BLOCK_SIZE] = {0};
    Byte n = 0;

    /* max len */
    if (NULL == out) {
        return outlen;
    }

    SM4_set_key(m_key, &stKey);
    memcpy(ivec, m_iv, DEF_SM4_BLOCK_SIZE);
    
    CRYPTO_cbc128_decrypt(input, output, len, &stKey, 
        ivec, (block128_f)SM4_decrypt);

    n = output[outlen-1];
    if (0 == n || n > (Byte)DEF_SM4_BLOCK_SIZE) {
        return -1;
    }

    for (Byte i = 0; i < n; i++) {
        if (n == output[--outlen]) {
            output[outlen] = 0;
        } else {
            return -1;
        }
    }

    return outlen;
}

Void printHex(const Char* prompt, const Void* data, int len) {
    const Byte* psz = (const Byte*)data;
    
    fprintf(stdout, "====%s| len=%d|\n", prompt, len);
    
    for (int i=0; i<len; ++i) {
        fprintf(stdout, "%02X", psz[i]);
    }

    fprintf(stdout, "\n");
}
