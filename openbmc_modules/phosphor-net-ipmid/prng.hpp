#include <openssl/rand.h>

namespace crypto
{

struct prng
{
    static unsigned int rand()
    {
        unsigned int v;
        RAND_bytes(reinterpret_cast<unsigned char*>(&v), sizeof(v));
        return v;
    }
};

} // namespace crypto
