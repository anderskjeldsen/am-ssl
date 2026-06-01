#include <libc/core.h>
#include <Am/Net/Ssl/SslSocketStream.h>
#include <amigaos/Am/Net/Ssl/SslSocketStream.h>
#include <Am/IO/Stream.h>
#include <Am/Lang/Object.h>
#include <Am/Net/Socket.h>
#include <Am/Lang/String.h>
#include <Am/Lang/Int.h>
#include <Am/Lang/Array.h>
#include <Am/Lang/Byte.h>
#include <Am/Lang/Long.h>
#include <Am/Lang/Exception.h>
#include <libc/core_inline_functions.h>

#include <amigaos/am_ssl_amissl_init.h>
#include <stdio.h>
#include <errno.h>
#include <proto/socket.h>
#include <sys/socket.h>

function_result Am_Net_Ssl_SslPrivate_f_openSslInitialized_0(void);
function_result Am_Net_Ssl_SslPrivate_f_setOpenSslInitialized_0(void);

// AmigaOS m68k SslSocketStream implementation against AmiSSL.
//
// Same shape as the libc version (src/native-c/libc/Am/Net/Ssl/SslSocketStream.c)
// — AmiSSL is API-compatible with OpenSSL 3.x, so SSL_CTX_new / SSL_new /
// SSL_connect / SSL_read / SSL_write etc. behave identically. The only
// difference is the bring-up: -lamisslauto isn't linked on m68k (its
// destructor freezes the system on shutdown when combined with the
// AmLang runtime), so each `_native_init_0` calls
// am_ssl_amissl_ensure_initialised() first. SSL_library_init() /
// SSL_load_error_strings() aren't needed — AmiSSL initialises its
// OpenSSL globals at OpenAmiSSLTags() time.
//
// We also skip CleanupAmiSSL on shutdown — see amissl_init.c. Per-stream
// resources (SSL_CTX, SSL, X509) ARE freed in _native_release_0 the
// normal way; only the library-level CloseAmiSSL is suppressed.

function_result Am_Net_Ssl_SslSocketStream__native_init_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    bool __returning = false;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    X509 *cert;
    long verify_result;
    aobject *socket_obj;
    aobject *host_name;
    string_holder *host_name_string_holder;
    int s;
    ssl_socket_stream_holder *holder;

    printf("Ssl: enter __native_init_0\n"); fflush(stdout);
    if (this != NULL) {
        __increase_reference_count(this);
    }
    printf("Ssl: about to call per-task ensure_initialised\n"); fflush(stdout);

    // Per-task AmiSSL bring-up has to happen BEFORE any OpenSSL call
    // (including OPENSSL_init_ssl below). amisslauto brought up the
    // main task at constructor time; on a worker task, AmiSSL has no
    // per-task slot for FindTask(NULL) and any OpenSSL function that
    // dispatches through AmiSSL hangs (verified: OPENSSL_init_ssl on
    // a worker without per-task AmiSSL silently never returns).
    //
    // Idempotent — no-op on the main task or on a worker already
    // brought up. The Thread finalizer that
    // `Am.Net.Ssl.SslSocketStream.nativeInit()` registers handles
    // matching cleanup at task exit.
    if (!am_ssl_amissl_ensure_initialised_for_current_task()) {
        __throw_simple_exception("Failed to initialise AmiSSL on this task", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __exit;
    }

    {
        printf("Ssl: openssl_state check\n"); fflush(stdout);
        function_result openssl_state = Am_Net_Ssl_SslPrivate_f_openSslInitialized_0();
        if (!openssl_state.return_value.value.bool_value) {
            printf("Ssl: ensure_initialised (OpenSSL global)\n"); fflush(stdout);
            if (!am_ssl_amissl_ensure_initialised()) {
                __throw_simple_exception("Failed to initialise AmiSSL", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
                goto __exit;
            }
            printf("Ssl: ensure_initialised done\n"); fflush(stdout);
            Am_Net_Ssl_SslPrivate_f_setOpenSslInitialized_0();
        }
    }
    printf("Ssl: TLS_client_method()\n"); fflush(stdout);
    const SSL_METHOD *m = TLS_client_method();
    printf("Ssl: TLS_client_method returned %p, SSL_CTX_new\n", (void *) m); fflush(stdout);
    ssl_ctx = SSL_CTX_new(m);
    printf("Ssl: SSL_CTX_new returned %p\n", (void *) ssl_ctx); fflush(stdout);
    if (ssl_ctx == NULL) {
        __throw_simple_exception("Failed to create SSL context", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __exit;
    }

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);

    if (!SSL_CTX_set_default_verify_paths(ssl_ctx)) {
        __throw_simple_exception("Failed to set default verify paths", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail2;
    }

    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        __throw_simple_exception("Failed to create SSL", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail2;
    }

    socket_obj = this->object_properties.class_object_properties.properties[Am_Net_Ssl_SslSocketStream_P_socket].nullable_value.value.object_value;
    s = socket_obj->object_properties.class_object_properties.object_data.value.int_value;
    printf("Ssl: socket fd from AmLang holder = %d\n", s); fflush(stdout);

    // Sanity check the fd is valid on THIS task before handing to AmiSSL.
    // If getpeername fails here with EBADF, the fd is bad in this task's
    // bsdsocket slot (e.g. socket was created on a different task) and
    // SSL_connect would also fail. If getpeername succeeds, the fd is
    // good and any later EBADF means AmiSSL is dispatching via the
    // wrong SocketBase / wrong task.
    {
        struct sockaddr peer;
        // socklen_t (not plain int) — the bsdsocket prototype takes a
        // long-typed pointer for the length arg, and `int *` and
        // `long *` are distinct pointer types under gcc even when
        // both are 32 bits on m68k. Using socklen_t matches Socket.c
        // and silences -Wincompatible-pointer-types.
        socklen_t peer_len = sizeof(peer);
        int gp_rc = getpeername(s, &peer, &peer_len);
        printf("Ssl: getpeername(fd=%d) returned %d (errno=%d)\n",
               s, gp_rc, errno); fflush(stdout);
    }

    if (!SSL_set_fd(ssl, s)) {
        __throw_simple_exception("Failed to set SSL file descriptor", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail3;
    }
    printf("Ssl: SSL_set_fd(fd=%d) ok; SSL_get_fd reports %d\n",
           s, SSL_get_fd(ssl)); fflush(stdout);

    host_name = this->object_properties.class_object_properties.properties[Am_Net_Ssl_SslSocketStream_P_hostName].nullable_value.value.object_value;
    host_name_string_holder = host_name->object_properties.class_object_properties.object_data.value.custom_value;

    if (!SSL_set_tlsext_host_name(ssl, host_name_string_holder->string_value)) {
        __throw_simple_exception("Failed to set SSL host name", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail4;
    }
    printf("Ssl: SNI set to %s\n", host_name_string_holder->string_value); fflush(stdout);

    {
        printf("Ssl: calling SSL_connect (fd=%d)\n", s); fflush(stdout);
        int connect_rc = SSL_connect(ssl);
        printf("Ssl: SSL_connect returned %d\n", connect_rc); fflush(stdout);
        if (connect_rc != 1) {
            // Pull the most recent error off OpenSSL's per-thread error
            // queue so the exception message tells us *why* the handshake
            // failed (cert verification, protocol mismatch, peer reset,
            // etc.). ERR_error_string_n writes a fixed-format diagnostic
            // like "error:0A000086:SSL routines::certificate verify failed"
            // — printable ASCII, safe to embed in the exception text.
            unsigned long err_code = ERR_peek_last_error();
            int ssl_err = SSL_get_error(ssl, connect_rc);
            int libc_errno = errno;
            int task_errno = am_ssl_amissl_current_task_errno();
            char err_msg[256];
            char details[384];
            if (err_code != 0) {
                ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
            } else {
                snprintf(err_msg, sizeof(err_msg), "no OpenSSL error queued");
            }
            snprintf(details, sizeof(details),
                     "SSL handshake failed: SSL_get_error=%d, libc_errno=%d, amissl_task_errno=%d, %s",
                     ssl_err, libc_errno, task_errno, err_msg);
            printf("Ssl: %s\n", details); fflush(stdout);
            __throw_simple_exception(details, "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
            goto __fail4;
        }
    }

    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL) {
        __throw_simple_exception("Failed to retrieve server certificate", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail4;
    }

    verify_result = SSL_get_verify_result(ssl);
    if (verify_result != X509_V_OK) {
        __throw_simple_exception("Certificate verification failed", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail5;
    }

    holder = calloc(1, sizeof(ssl_socket_stream_holder));
    holder->ssl_ctx = ssl_ctx;
    holder->ssl = ssl;
    holder->cert = cert;
    this->object_properties.class_object_properties.object_data.value.custom_value = holder;

    goto __exit;
__fail5: ;
    X509_free(cert);
__fail4: ;
    SSL_shutdown(ssl);
__fail3: ;
    SSL_free(ssl);
__fail2: ;
    SSL_CTX_free(ssl_ctx);
__exit: ;
    if (this != NULL) {
        __decrease_reference_count(this);
    }
    return __result;
}

function_result Am_Net_Ssl_SslSocketStream__native_release_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    ssl_socket_stream_holder *holder;

    holder = this->object_properties.class_object_properties.object_data.value.custom_value;

    if (holder != NULL) {
        X509_free(holder->cert);
        SSL_shutdown(holder->ssl);
        SSL_free(holder->ssl);
        SSL_CTX_free(holder->ssl_ctx);
    }

__exit: ;
    return __result;
}

function_result Am_Net_Ssl_SslSocketStream__native_mark_children_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
__exit: ;
    return __result;
}

function_result Am_Net_Ssl_SslSocketStream_read_0(aobject * const this, aobject * buffer, long long offset, int length)
{
    function_result __result = { .has_return_value = true };
    bool __returning = false;
    ssl_socket_stream_holder *holder;
    array_holder *a_holder;
    int received;

    if (this != NULL) {
        __increase_reference_count(this);
    }
    if (buffer != NULL) {
        __increase_reference_count(buffer);
    }
    holder = this->object_properties.class_object_properties.object_data.value.custom_value;

    if (holder != NULL) {
        a_holder = buffer->object_properties.class_object_properties.object_data.value.custom_value;

        if (length > a_holder->size) {
            __throw_simple_exception("Receive length is bigger than array", "in Am_Net_Ssl_SslSocketStream_read_0", &__result);
            goto __exit;
        }

        received = SSL_read(holder->ssl, a_holder->array_data, length);
        __result.return_value.value.int_value = received;
        __result.return_value.flags = PRIMITIVE_INT;
        __returning = true;
    }

__exit: ;
    if (this != NULL) {
        __decrease_reference_count(this);
    }
    if (buffer != NULL) {
        __decrease_reference_count(buffer);
    }
    return __result;
}

function_result Am_Net_Ssl_SslSocketStream_write_0(aobject * const this, aobject * buffer, long long offset, int length)
{
    function_result __result = { .has_return_value = false };
    bool __returning = false;
    ssl_socket_stream_holder *holder;
    array_holder *a_holder;
    int sent;

    if (this != NULL) {
        __increase_reference_count(this);
    }
    if (buffer != NULL) {
        __increase_reference_count(buffer);
    }
    holder = this->object_properties.class_object_properties.object_data.value.custom_value;

    if (holder != NULL) {
        a_holder = buffer->object_properties.class_object_properties.object_data.value.custom_value;

        if (length > a_holder->size) {
            __throw_simple_exception("Send length is bigger than array", "in Am_Net_Ssl_SslSocketStream_write_0", &__result);
            __returning = true;
            goto __exit;
        }

        sent = SSL_write(holder->ssl, a_holder->array_data, length);
        __result.return_value.value.int_value = sent;
        __result.return_value.flags = PRIMITIVE_UINT;
    }
__exit: ;
    if (this != NULL) {
        __decrease_reference_count(this);
    }
    if (buffer != NULL) {
        __decrease_reference_count(buffer);
    }
    return __result;
}

// Dispatched by the Thread finalizer that
// `Am.Net.Ssl.SslSocketStream.nativeInit()` registered. Runs from
// inside the worker task that opened, so the FindTask(NULL) lookup in
// am_ssl_amissl_close_for_current_task resolves to the right task.
function_result Am_Net_Ssl_SslSocketStream_closeAmiSSLForThread_0(void)
{
    function_result __result = { .has_return_value = false };
    am_ssl_amissl_close_for_current_task();
    return __result;
}

// Force OPENSSL_init_ssl + RAND_seed to run on the calling task. Used
// by application code that knows it will do SSL only from worker
// threads — call from main to pre-warm so the worker doesn't trip on
// the OPENSSL_init_ssl hang. Idempotent; flips the AmLang-side
// `SslPrivate.openSslInitialized` flag so the regular
// `SslSocketStream._native_init_0` lazy path becomes a no-op.
function_result Am_Net_Ssl_SslSocketStream_warmCurrentTask_0(void)
{
    function_result __result = { .has_return_value = false };
    function_result openssl_state = Am_Net_Ssl_SslPrivate_f_openSslInitialized_0();
    if (!openssl_state.return_value.value.bool_value) {
        am_ssl_amissl_ensure_initialised();
        Am_Net_Ssl_SslPrivate_f_setOpenSslInitialized_0();
    }
    return __result;
}
