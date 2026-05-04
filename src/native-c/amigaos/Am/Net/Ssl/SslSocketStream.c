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

#include <amigaos/amissl_init.h>
#include <stdio.h>

// AmigaOS m68k SslSocketStream implementation against AmiSSL.
//
// Same shape as the libc version (src/native-c/libc/Am/Net/Ssl/SslSocketStream.c)
// — AmiSSL is API-compatible with OpenSSL 3.x, so SSL_CTX_new / SSL_new /
// SSL_connect / SSL_read / SSL_write etc. behave identically. The only
// difference is the bring-up: -lamisslauto isn't linked on m68k (its
// destructor freezes the system on shutdown when combined with the
// AmLang runtime), so each `_native_init_0` calls
// amissl_ensure_initialised() first. SSL_library_init() /
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

    if (this != NULL) {
        __increase_reference_count(this);
    }

    // Breadcrumbs to localise the crash that's happening somewhere
    // in this function. Each step prints + flushes BEFORE the call,
    // so whatever step the user sees as the LAST printed line is
    // the one that crashed mid-call.
    printf("ssl_init: amissl_ensure_initialised...\n"); fflush(stdout);
    if (!amissl_ensure_initialised()) {
        __throw_simple_exception("Failed to initialise AmiSSL", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __exit;
    }

    printf("ssl_init: SSL_CTX_new...\n"); fflush(stdout);
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (ssl_ctx == NULL) {
        __throw_simple_exception("Failed to create SSL context", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __exit;
    }

    printf("ssl_init: SSL_CTX_set_verify...\n"); fflush(stdout);
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);

    printf("ssl_init: SSL_CTX_set_default_verify_paths...\n"); fflush(stdout);
    if (!SSL_CTX_set_default_verify_paths(ssl_ctx)) {
        __throw_simple_exception("Failed to set default verify paths", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail2;
    }

    printf("ssl_init: SSL_new...\n"); fflush(stdout);
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        __throw_simple_exception("Failed to create SSL", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail2;
    }

    printf("ssl_init: read socket fd from AmLang object...\n"); fflush(stdout);
    socket_obj = this->object_properties.class_object_properties.properties[Am_Net_Ssl_SslSocketStream_P_socket].nullable_value.value.object_value;
    s = socket_obj->object_properties.class_object_properties.object_data.value.int_value;
    printf("ssl_init: socket fd=%d, SSL_set_fd...\n", s); fflush(stdout);

    if (!SSL_set_fd(ssl, s)) {
        __throw_simple_exception("Failed to set SSL file descriptor", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail3;
    }

    printf("ssl_init: read hostName from AmLang object...\n"); fflush(stdout);
    host_name = this->object_properties.class_object_properties.properties[Am_Net_Ssl_SslSocketStream_P_hostName].nullable_value.value.object_value;
    host_name_string_holder = host_name->object_properties.class_object_properties.object_data.value.custom_value;
    printf("ssl_init: hostName='%s', SSL_set_tlsext_host_name...\n", host_name_string_holder->string_value); fflush(stdout);

    if (!SSL_set_tlsext_host_name(ssl, host_name_string_holder->string_value)) {
        __throw_simple_exception("Failed to set SSL host name", "in Am_Net_Ssl_SslSocketStream__native_init_0", &__result);
        goto __fail4;
    }
    printf("ssl_init: SSL_connect...\n"); fflush(stdout);

    {
        int connect_rc = SSL_connect(ssl);
        if (connect_rc != 1) {
            // Pull the most recent error off OpenSSL's per-thread error
            // queue so the exception message tells us *why* the handshake
            // failed (cert verification, protocol mismatch, peer reset,
            // etc.). ERR_error_string_n writes a fixed-format diagnostic
            // like "error:0A000086:SSL routines::certificate verify failed"
            // — printable ASCII, safe to embed in the exception text.
            unsigned long err_code = ERR_peek_last_error();
            int ssl_err = SSL_get_error(ssl, connect_rc);
            char err_msg[256];
            char details[320];
            if (err_code != 0) {
                ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
            } else {
                snprintf(err_msg, sizeof(err_msg), "no OpenSSL error queued");
            }
            snprintf(details, sizeof(details),
                     "SSL handshake failed: SSL_get_error=%d, %s",
                     ssl_err, err_msg);
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
