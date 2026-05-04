/*
 * Pure-C TLS client for AmigaOS m68k via AmiSSL — minimal control test
 * for the AmLang ssltest example.
 *
 * Modeled directly on AmiSSL's own test/https.c, which works on this
 * Amiga. We do manual init (OpenLibrary + OpenAmiSSLTags) and use
 * <proto/socket.h> so socket()/connect()/send()/recv()/CloseSocket()
 * all dispatch through SocketBase — the exact same path AmiSSL uses
 * internally for SSL_read/SSL_write. No -lamisslauto.
 *
 * Two phases:
 *   Phase 1: plain HTTP GET on port 80 (TCP only, no TLS).
 *   Phase 2: HTTPS GET on port 443 via AmiSSL.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>
#include <libraries/amissl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#define HOST "github.com"
#define PATH "/"

// AmiSSL's bring-up needs these globals visible to its inline headers.
struct Library *SocketBase        = NULL;
struct Library *AmiSSLMasterBase  = NULL;
struct Library *AmiSSLBase        = NULL;
struct Library *AmiSSLExtBase     = NULL;

static int  amissl_open(void);
static void amissl_close(void);
static void seed_rand(void);

static int tcp_connect(int port)
{
    struct hostent *he;
    struct sockaddr_in addr;
    int sock;

    printf("tcp_connect: resolving %s\n", HOST);
    he = gethostbyname((STRPTR)HOST);
    if (!he) {
        printf("FAIL gethostbyname errno=%d\n", errno);
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("FAIL socket errno=%d\n", errno);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_len    = he->h_length;   // mirrors https.c — bsdsocket
                                      // expects a non-zero sin_len
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    printf("tcp_connect: connecting %s:%d\n", HOST, port);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("FAIL connect errno=%d\n", errno);
        CloseSocket(sock);
        return -1;
    }
    printf("tcp_connect: TCP connected (fd=%d)\n", sock);
    return sock;
}

static int phase1_http(void)
{
    int sock;
    char buf[1024];
    int n;
    int total = 0;

    printf("\n=== phase 1: plain HTTP on port 80 ===\n");

    sock = tcp_connect(80);
    if (sock < 0) {
        return 1;
    }

    snprintf(buf, sizeof(buf),
             "GET " PATH " HTTP/1.0\r\nHost: " HOST "\r\nConnection: close\r\n\r\n");
    n = send(sock, buf, strlen(buf), 0);
    printf("phase1: send %d bytes\n", n);
    if (n < 0) {
        printf("FAIL send errno=%d\n", errno);
        CloseSocket(sock);
        return 1;
    }

    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        if (total == 0) {
            printf("phase1: recv %d bytes:\n%s\n", n, buf);
        }
        total += n;
    }
    if (n < 0) {
        printf("FAIL recv errno=%d after %d bytes\n", errno, total);
    } else {
        printf("phase1: total %d bytes received, peer closed cleanly\n", total);
    }

    CloseSocket(sock);
    return 0;
}

static int phase2_https(void)
{
    int sock = -1;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    char buf[1024];
    int n;
    int rc;

    printf("\n=== phase 2: HTTPS via AmiSSL on port 443 ===\n");

    OPENSSL_init_ssl(
        OPENSSL_INIT_SSL_DEFAULT
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        NULL);
    seed_rand();

    sock = tcp_connect(443);
    if (sock < 0) {
        return 1;
    }

    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        printf("FAIL SSL_CTX_new\n");
        CloseSocket(sock);
        return 1;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        printf("WARN SSL_CTX_set_default_verify_paths failed (continuing)\n");
    }

    ssl = SSL_new(ctx);
    if (!ssl) {
        printf("FAIL SSL_new\n");
        SSL_CTX_free(ctx);
        CloseSocket(sock);
        return 1;
    }

    if (!SSL_set_fd(ssl, sock)) {
        printf("FAIL SSL_set_fd\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CloseSocket(sock);
        return 1;
    }
    if (!SSL_set_tlsext_host_name(ssl, HOST)) {
        printf("FAIL SSL_set_tlsext_host_name\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CloseSocket(sock);
        return 1;
    }

    printf("phase2: SSL_connect...\n");
    errno = 0;
    rc = SSL_connect(ssl);
    if (rc != 1) {
        int err = SSL_get_error(ssl, rc);
        int saved_errno = errno;
        unsigned long e = ERR_peek_last_error();
        char emsg[256];
        if (e != 0) {
            ERR_error_string_n(e, emsg, sizeof(emsg));
            printf("FAIL SSL_connect: rc=%d err=%d errno=%d %s\n", rc, err, saved_errno, emsg);
        } else {
            printf("FAIL SSL_connect: rc=%d err=%d errno=%d (no OpenSSL error; %s)\n",
                   rc, err, saved_errno,
                   saved_errno == 0 ? "peer closed during handshake" : "bsdsocket syscall failed");
        }
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CloseSocket(sock);
        return 1;
    }
    printf("phase2: handshake OK, cipher=%s\n", SSL_get_cipher(ssl));

    {
        long vr = SSL_get_verify_result(ssl);
        if (vr != X509_V_OK) {
            printf("WARN cert verify result=%ld (continuing)\n", vr);
        }
    }

    snprintf(buf, sizeof(buf),
             "GET " PATH " HTTP/1.0\r\nHost: " HOST "\r\nConnection: close\r\n\r\n");
    n = SSL_write(ssl, buf, strlen(buf));
    printf("phase2: SSL_write %d bytes\n", n);

    n = SSL_read(ssl, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("phase2: SSL_read %d bytes:\n%s\n", n, buf);
    } else {
        printf("phase2: SSL_read returned %d\n", n);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    CloseSocket(sock);
    return 0;
}

int main(void)
{
    int phase1_rc = 1, phase2_rc = 1;

    if (!amissl_open()) {
        printf("FAIL amissl_open\n");
        return 1;
    }

    phase1_rc = phase1_http();
    phase2_rc = phase2_https();

    amissl_close();

    printf("\nssl-c-test: done (phase1 rc=%d, phase2 rc=%d)\n", phase1_rc, phase2_rc);
    return phase1_rc | phase2_rc;
}

// Manual init mirroring AmiSSL's test/https.c. We don't link
// -lamisslauto — instead we open bsdsocket / amisslmaster / amissl
// here and close them in amissl_close(). Returns 1 on success, 0 on
// failure (with a printed reason).
static int amissl_open(void)
{
    LONG err;

    SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4);
    if (!SocketBase) {
        printf("amissl_open: couldn't open bsdsocket.library v4\n");
        return 0;
    }

    AmiSSLMasterBase = OpenLibrary((STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION);
    if (!AmiSSLMasterBase) {
        printf("amissl_open: couldn't open amisslmaster.library\n");
        return 0;
    }

    err = OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                         AmiSSL_UsesOpenSSLStructs, TRUE,
                         AmiSSL_GetAmiSSLBase,    (Tag)&AmiSSLBase,
                         AmiSSL_GetAmiSSLExtBase, (Tag)&AmiSSLExtBase,
                         AmiSSL_SocketBase,       (Tag)SocketBase,
                         AmiSSL_ErrNoPtr,         (Tag)&errno,
                         TAG_DONE);
    if (err != 0) {
        printf("amissl_open: OpenAmiSSLTags returned %ld\n", (long)err);
        return 0;
    }
    return 1;
}

static void amissl_close(void)
{
    if (AmiSSLBase) {
        CloseAmiSSL();
        AmiSSLBase    = NULL;
        AmiSSLExtBase = NULL;
    }
    if (AmiSSLMasterBase) {
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
    }
    if (SocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
}

static void seed_rand(void)
{
    unsigned char buf[256];
    unsigned int i;
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (unsigned char)(rand() & 0xFF);
    }
    RAND_seed(buf, sizeof(buf));
}
