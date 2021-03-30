#include "openssl_alloc.hpp"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#include <string.h>

static void* OPENSSL_zalloc(size_t num)
{
    void* ret = OPENSSL_malloc(num);

    if (ret != NULL)
    {
        memset(ret, 0, num);
    }
    return ret;
}

EVP_MD_CTX* EVP_MD_CTX_new(void)
{
    return (EVP_MD_CTX*)OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

void EVP_MD_CTX_free(EVP_MD_CTX* ctx)
{
    EVP_MD_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
}

#endif
