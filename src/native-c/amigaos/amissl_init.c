// Shared AmiSSL bring-up for AmigaOS m68k.
//
// Why this exists rather than just linking -lamisslauto:
//   amisslauto installs an __attribute__((destructor)) that calls
//   CloseAmiSSL + CloseLibrary(amisslmaster) + CloseLibrary(bsdsocket)
//   AFTER `main` has returned. With the AmLang runtime in the link
//   line, that destructor freezes the m68k system on shutdown
//   (presumably interacting badly with libnix's -lsocket destructor,
//   which also closes bsdsocket from its own LIFO destructor slot,
//   and/or with how AmLang has torn things down by then).
//
//   The pure-C sha1-c-test program with the *same* library list
//   (-lm -lsocket -lamisslstubs -lamisslauto) exits cleanly, so
//   amisslauto in isolation is fine; it's the combination with the
//   AmLang runtime's shutdown chain that hangs.
//
// What we do instead:
//   - Define the four library-base globals strong here, once per
//     link line. AmiSSL's inline headers find them via baserel.
//   - Open the libraries lazily on the first call from any native
//     function (currently Sha1.digest, eventually SslSocketStream's
//     read/write etc.) by calling amissl_ensure_initialised().
//   - Never close anything. The OS reclaims library opens when the
//     process exits — slightly leaky for a long-lived program but
//     fine for one-shot CLI runs and avoids the shutdown freeze.

#include <amigaos/amissl_init.h>

#include <proto/exec.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>
#include <libraries/amissl.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdlib.h>    // rand() — used by seed_rand below.
#include <errno.h>     // for the strong `int errno` defined in errno.c;
                       // AmiSSL needs a pointer to it so its bsdsocket
                       // wrappers can surface system-call errors.

// AmigaOS shell "magic stack cookie" — when the binary is launched
// from the shell, AmigaOS scans the program's data section for the
// string "$STACK:N" and uses N as the per-process stack size. Default
// stack is around 4 KB which is *way* too small for OpenSSL 3.x's TLS
// handshake (cipher state machines + cert chain validation are
// stack-heavy). 32 KB is a comfortable margin; bump higher if you see
// crashes deeper in the SSL_connect / SSL_read path. AmiSSL's own
// test/https.c uses 8 KB; we err on the safe side.
const char __amissl_stack_cookie[] = "$STACK:32768";

struct Library *AmiSSLMasterBase = NULL;
struct Library *AmiSSLBase = NULL;
struct Library *AmiSSLExtBase = NULL;
struct Library *SocketBase = NULL;

static int amissl_initialised   = 0;
static int we_opened_socket     = 0; // bsdsocket.library — only close if we
                                     //   were the ones who OpenLibrary'd it.
                                     //   libnix's -lsocket may have already
                                     //   opened it via its own constructor.
static int we_opened_master     = 0; // amisslmaster.library — we always own
                                     //   this one, but flag for symmetry.
static int we_opened_amissl     = 0; // OpenAmiSSLTags succeeded → CloseAmiSSL
                                     //   on release.

// Weak-entropy seed for OpenSSL's RNG. m68k AmigaOS has no
// /dev/urandom, so without explicit seeding the TLS handshake
// silently produces an empty ClientHello.random and fails with
// SSL_ERROR_SYSCALL + no queued OpenSSL error. Mirrors AmiSSL's own
// test/https.c — which is fine for a diagnostic but for real
// production use you'd want to mix in a hardware RNG / accumulated
// task timing / DateStamp etc.
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
    LONG err;

    if (amissl_initialised) {
        return 1;
    }

    if (!SocketBase) {
        SocketBase = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
        if (!SocketBase) {
            return 0;
        }
        we_opened_socket = 1;
    }

    if (!AmiSSLMasterBase) {
        AmiSSLMasterBase = OpenLibrary((CONST_STRPTR)"amisslmaster.library",
                                       AMISSLMASTER_MIN_VERSION);
        if (!AmiSSLMasterBase) {
            return 0;
        }
        we_opened_master = 1;
    }

    // OpenAmiSSLTags is variadic; values are stuffed into a Tag array
    // (Tag = ULONG) so pointer arguments need an explicit (Tag) cast
    // to silence -Wint-conversion.
    err = OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                         AmiSSL_UsesOpenSSLStructs, TRUE,
                         AmiSSL_GetAmiSSLBase,    (Tag)&AmiSSLBase,
                         AmiSSL_GetAmiSSLExtBase, (Tag)&AmiSSLExtBase,
                         AmiSSL_SocketBase,       (Tag)SocketBase,
                         AmiSSL_ErrNoPtr,         (Tag)&errno,
                         TAG_DONE);
    if (err != 0) {
        return 0;
    }
    we_opened_amissl = 1;

    // Two calls AmiSSL's own test/https.c does after OpenAmiSSLTags,
    // and which `-lamisslauto`'s constructor did NOT do for us. Both
    // verified necessary on m68k by stripping ssl-c-test down until
    // it matched https.c:
    //   1) OPENSSL_init_ssl with ADD_ALL_CIPHERS|ADD_ALL_DIGESTS
    //      populates the EVP cipher table. Without it, SSL_CTX_new's
    //      cipher list is empty/minimal, ClientHello goes out with
    //      no acceptable ciphers, the server (e.g. github) drops the
    //      connection silently → SSL_ERROR_SYSCALL with no queued
    //      OpenSSL error. Has no effect on Sha1, but cheap to call
    //      always so we don't have to track which APIs the caller
    //      will use.
    //   2) RAND_seed gives the RNG something to start with — see
    //      seed_rand() comment above.
    OPENSSL_init_ssl(
        OPENSSL_INIT_SSL_DEFAULT
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        NULL);
    seed_rand();

    amissl_initialised = 1;
    return 1;
}

void amissl_release_if_initialised(void)
{
    // Idempotent: if init was never run (or already released), this
    // is a no-op. Close in reverse order of opening, mirroring the
    // sequence amisslauto's destructor uses — but inside main, before
    // the AmLang runtime has torn stdio + memory pools down.

    if (!amissl_initialised) {
        return;
    }

    if (we_opened_amissl) {
        CloseAmiSSL();
        we_opened_amissl = 0;
    }
    AmiSSLBase    = NULL;
    AmiSSLExtBase = NULL;

    if (we_opened_master && AmiSSLMasterBase) {
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
        we_opened_master = 0;
    }

    if (we_opened_socket && SocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
        we_opened_socket = 0;
    }

    amissl_initialised = 0;
}
