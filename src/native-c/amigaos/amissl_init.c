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
// So `amissl_ensure_initialised()` here is a thin idempotent wrapper
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

#include <amigaos/amissl_init.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdlib.h>    // rand()

// AmigaOS shell "magic stack cookie" — when the binary is launched
// from the shell, AmigaOS scans the program's data section for the
// string "$STACK:N" and uses N as the per-process stack size. Default
// stack on m68k is around 4 KB which is way too small for OpenSSL's
// TLS handshake; 32 KB is a comfortable margin.
const char __amissl_stack_cookie[] = "$STACK:32768";

static int openssl_initialised = 0;

static void seed_rand(void)
{
    unsigned char buf[256];
    unsigned int i;
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (unsigned char)(rand() & 0xFF);
    }
    RAND_seed(buf, sizeof(buf));
}

int amissl_ensure_initialised(void)
{
    if (openssl_initialised) {
        return 1;
    }

    OPENSSL_init_ssl(
        OPENSSL_INIT_SSL_DEFAULT
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        NULL);
    seed_rand();

    openssl_initialised = 1;
    return 1;
}
