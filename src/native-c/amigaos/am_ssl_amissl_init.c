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

#include <libc/core.h>
#include <amigaos/am_ssl_amissl_init.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdlib.h>    // rand()

#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>

extern struct Library *SocketBase;       // owned by am-net's Socket.c
extern struct Library *AmiSSLMasterBase; // owned by amisslauto
extern struct Library *AmiSSLBase;       // owned by amisslauto

// Per-task bsdsocket base accessor exported from am-net's Socket.c.
// Returns the SocketBase THIS task got from its own OpenLibrary
// (different from the global on Roadshow/Miami/amiberry), or NULL
// on the main task / a task that never opened bsdsocket.
extern struct Library *am_net_current_task_socket_base(void);

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
    // Process-global OpenSSL init. Without these two calls the EVP
    // cipher table is empty and ClientHello.random is unseeded — the
    // peer drops the connection mid-handshake and SSL_connect returns
    // SSL_ERROR_SYSCALL with an empty error queue (exactly the
    // symptom hit on 2026-05-27).
    //
    // NOTE: OPENSSL_init_ssl hangs on amiberry (the symptom isolated
    // earlier — the hang is inside OpenSSL's process-global init, not
    // in per-task setup). FS-UAE doesn't hit it. If you need to run
    // an SSL build under amiberry you'll need to track down whatever
    // makes the OpenSSL global init loop — don't put the stub back as
    // a workaround.
    OPENSSL_init_ssl(
        OPENSSL_INIT_SSL_DEFAULT
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        NULL);
    seed_rand();
    return 1;
}

// Per-AmLang-Thread AmiSSL bring-up tracker. Same shape as the
// bsdsocket tracker in am-net's amigaos Socket.c — keyed by the
// AmLang Thread aobject (FindTask(NULL)->tc_UserData) so the
// nativeInit registration and the finalizer cleanup agree on the
// task identity, even if FindTask is queried at different points.
//
// `errno_slot` is the per-task `int errno` we hand AmiSSL via
// InitAmiSSL's AmiSSL_ErrNoPtr tag — AmiSSL writes the calling task's
// last error there, and we need a separate location per task so two
// workers don't trample each other. Allocated once on bring-up,
// freed in close. AmiSSL keeps the pointer alive between
// InitAmiSSL and CleanupAmiSSL.
struct ssl_task_node {
    aobject *thread;
    int errno_slot;
    struct ssl_task_node *next;
};

static struct ssl_task_node *ssl_task_list = NULL;

static aobject *current_amlang_thread(void)
{
    struct Task *task = FindTask(NULL);
    if (task == NULL) {
        return NULL;
    }
    return (aobject *) task->tc_UserData;
}

static struct ssl_task_node *ssl_task_lookup(aobject *thread)
{
    struct ssl_task_node *n;
    struct ssl_task_node *found = NULL;
    Forbid();
    for (n = ssl_task_list; n != NULL; n = n->next) {
        if (n->thread == thread) {
            found = n;
            break;
        }
    }
    Permit();
    return found;
}

static struct ssl_task_node *ssl_task_register(aobject *thread)
{
    struct ssl_task_node *node = (struct ssl_task_node *) malloc(sizeof(struct ssl_task_node));
    if (node == NULL) {
        return NULL;
    }
    node->thread = thread;
    node->errno_slot = 0;
    Forbid();
    node->next = ssl_task_list;
    ssl_task_list = node;
    Permit();
    return node;
}

static struct ssl_task_node *ssl_task_unregister(aobject *thread)
{
    struct ssl_task_node *prev = NULL;
    struct ssl_task_node *n;
    Forbid();
    for (n = ssl_task_list; n != NULL; n = n->next) {
        if (n->thread == thread) {
            if (prev == NULL) {
                ssl_task_list = n->next;
            } else {
                prev->next = n->next;
            }
            break;
        }
        prev = n;
    }
    Permit();
    return n;   // caller frees
}

// Forward declared so the bring-up path below can call back into the
// AmLang Socket layer to register the per-thread finalizer. The
// symbol name follows the generated `<Class>_f_<method>_<index>`
// convention for non-native AmLang static methods.
extern function_result Am_Net_Ssl_SslSocketStream_f_nativeInit_0(void);

int am_ssl_amissl_ensure_initialised_for_current_task(void)
{
    aobject *thread;
    struct ssl_task_node *node;

    thread = current_amlang_thread();
    if (thread == NULL) {
        // Main task / non-AmLang task — amisslauto's constructor
        // already brought this one up. Nothing to do.
        return 1;
    }

    if (ssl_task_lookup(thread) != NULL) {
        // Worker has already been brought up.
        return 1;
    }

    // PRIOR EXPERIMENT (2026-05-27): the documented pattern was
    // OpenLibrary("amisslmaster.library") + InitAmiSSL +
    // CleanupAmiSSL + CloseLibrary per worker. Skipping it entirely
    // exits cleanly for SHA-1 but `SSL_CTX_new` fails on a worker —
    // SSL handshake state really does need a per-task InitAmiSSL.
    // The destructor Guru that the full pattern caused was traced to
    // the OpenLibrary/CloseLibrary half: amisslauto opens
    // amisslmaster.library once in its constructor and closes it
    // once in its destructor, and our extra per-worker open/close
    // pair disturbed something in that path. So this trim keeps
    // ONLY the InitAmiSSL / CleanupAmiSSL pair and shares
    // amisslauto's master/amissl/socket opens with no extra
    // OpenLibrary / CloseLibrary of our own.
    node = ssl_task_register(thread);
    if (node == NULL) {
        return 0;
    }

    // InitAmiSSL associates this task's errno location and SocketBase
    // with AmiSSL's per-task state. Per the AmiSSL developer (and the
    // amissl.library docs): "You can pass in process specific
    // ISocket or SocketBase using InitAmiSSL()." — and on
    // Roadshow/Miami/amiberry's default semantics the per-task
    // bsdsocket base is genuinely different from the main-task base.
    // Use this worker's OWN base from am-net's per-task tracker if
    // it's been opened here; fall back to the global only on the
    // main task / unmanaged tasks.
    //
    // node->errno_slot lives in the heap-allocated ssl_task_node so
    // each worker gets its own — am-ssl/cleanup frees it via the
    // Thread finalizer.
    struct Library *task_socket_base = am_net_current_task_socket_base();
    if (task_socket_base == NULL) {
        task_socket_base = SocketBase;
    }
    if (InitAmiSSL(AmiSSL_ErrNoPtr, (Tag) &node->errno_slot,
                   AmiSSL_SocketBase, (Tag) task_socket_base,
                   TAG_DONE) != 0) {
        ssl_task_unregister(thread);
        free(node);
        return 0;
    }

    // Register the Thread finalizer so the worker's CleanupAmiSSL
    // runs from inside the same task at exit. The call flips through
    // AmLang so the lambda hits the normal codegen path.
    Am_Net_Ssl_SslSocketStream_f_nativeInit_0();

    return 1;
}

int am_ssl_amissl_current_task_errno(void)
{
    aobject *thread = current_amlang_thread();
    struct ssl_task_node *node;
    if (thread == NULL) {
        return 0;
    }
    node = ssl_task_lookup(thread);
    if (node == NULL) {
        return 0;
    }
    return node->errno_slot;
}

void am_ssl_amissl_close_for_current_task(void)
{
    aobject *thread = current_amlang_thread();
    struct ssl_task_node *node;
    if (thread == NULL) {
        return;
    }
    node = ssl_task_unregister(thread);
    if (node == NULL) {
        return;
    }
    // CleanupAmiSSL releases this task's per-task SSL contexts and
    // disassociates the errno pointer we set in InitAmiSSL. NO
    // CloseLibrary on amisslmaster — we never opened it from this
    // worker; amisslauto owns the master/amissl/socket opens for the
    // whole process. The previous version that did call
    // CloseLibrary(AmiSSLMasterBase) here was what made amisslauto's
    // destructor Guru at process exit.
    CleanupAmiSSL(TAG_DONE);
    free(node);
}
