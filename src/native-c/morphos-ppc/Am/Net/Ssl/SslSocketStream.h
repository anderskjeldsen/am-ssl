#pragma once
#include <libc/core.h>
#include <Am/Net/Ssl/SslSocketStream.h>
#include <Am/IO/Stream.h>
#include <Am/Lang/Object.h>
#include <Am/Net/Socket.h>
#include <Am/Lang/Int.h>
#include <Am/Lang/Array.h>
#include <Am/Lang/Byte.h>
#include <Am/Lang/Long.h>
#include <Am/Lang/Exception.h>
#include <Am/Lang/String.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Mirrors libc's holder layout. AmiSSL exposes the same SSL_CTX / SSL /
// X509 types as upstream OpenSSL, so we can share the wire shape.
struct _ssl_socket_stream_holder {
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    X509 *cert;
};

typedef struct _ssl_socket_stream_holder ssl_socket_stream_holder;