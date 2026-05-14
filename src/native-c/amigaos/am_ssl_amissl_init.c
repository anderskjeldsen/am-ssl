// One-shot OpenSSL-level setup for AmigaOS m68k.
//
// Library bring-up (bsdsocket.library, amisslmaster.library, AmiSSL
// itself) is handled by `-lamisslauto`'s constructor before main runs.
// What amisslauto does NOT do is the OpenSSL-level init that AmiSSL's
// own test/https.c performs explicitly — without those two calls,
// `SSL_CTX_new` produces a context with no usable cipher list and the
// TLS handshake silently fails (peer closes connection during
// ClientHello). Verified by stripping ssl-c-test down to match
// https.c.
//
// So `am_ssl_amissl_ensure_initialised()` here is a thin idempotent wrapper
// that calls:
//   1) OPENSSL_init_ssl with ADD_ALL_CIPHERS|ADD_ALL_DIGESTS — populates
//      the EVP cipher table.
//   2) RAND_seed with weak entropy — m68k has no /dev/urandom and the
//      handshake needs random bytes for ClientHello.random and
//      ephemeral keys.
//
// Library globals (AmiSSLBase / AmiSSLExtBase / AmiSSLMasterBase /
// SocketBase) are NOT defined here any more — amisslauto declares them
// WEAK and we let it own them. am-net's amigaos Socket.c also weak-
// defines SocketBase, so when both are linked everyone shares the same
// global through amisslauto's strong-after-link resolution.

#include <amigaos/am_ssl_amissl_init.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdlib.h>    // rand()

static void seed_rand(void)
{
    unsigned char buf[256];
    unsigned int i;
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (unsigned char)(rand() & 0xFF);
    }
    RAND_seed(buf, sizeof(buf));
}

int am_ssl_amissl_ensure_initialised(void)
{
    OPENSSL_init_ssl(
        OPENSSL_INIT_SSL_DEFAULT
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        NULL);
    seed_rand();
    return 1;
}
