#include <libc/core.h>
#include <Am/Net/Ssl/SslSocketStream.h>
#include <aros/Am/Net/Ssl/SslSocketStream.h>
#include <Am/IO/Stream.h>
#include <Am/Lang/Object.h>
#include <Am/Net/Socket.h>
#include <Am/Lang/Int.h>
#include <Am/Lang/Array.h>
#include <Am/Lang/Byte.h>
#include <Am/Lang/Long.h>
#include <Am/Lang/Exception.h>
#include <Am/Lang/String.h>
#include <libc/core_inline_functions.h>

// AROS aarch64: no OpenSSL for the target. The SDK image only carries the
// HOST's x86-64 openssl headers, and there is no aarch64-aros libssl to link
// against, so this platform gets an explicit unsupported implementation
// instead of the libc/OpenSSL one.
//
// Every entry point either succeeds as a no-op (lifecycle) or throws so a
// caller trying to speak TLS fails loudly at the first read/write rather than
// silently exchanging plaintext. Swap this file for a real port once an
// aarch64-aros OpenSSL (or an AROS AmiSSL equivalent) exists — the AmLang side
// needs no changes.

static const char * const AM_AROS_NO_TLS =
	"TLS is not available on AROS aarch64 (no target OpenSSL build)";

function_result Am_Net_Ssl_SslSocketStream__native_init_0(aobject * const this)
{
	function_result __result = { .has_return_value = false };
	// Mirror the real implementation's ownership contract: _native_init
	// takes a reference that _native_release drops.
	if (this != NULL) {
		__increase_reference_count(this);
	}
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream__native_release_0(aobject * const this)
{
	function_result __result = { .has_return_value = false };
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream__native_mark_children_0(aobject * const this)
{
	function_result __result = { .has_return_value = false };
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream_read_0(aobject * const this, aobject * buffer, long long offset, unsigned int length)
{
	function_result __result = { .has_return_value = true };
	__throw_simple_exception(AM_AROS_NO_TLS, "in Am_Net_Ssl_SslSocketStream_read_0", &__result);
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream_write_0(aobject * const this, aobject * buffer, long long offset, unsigned int length)
{
	function_result __result = { .has_return_value = false };
	__throw_simple_exception(AM_AROS_NO_TLS, "in Am_Net_Ssl_SslSocketStream_write_0", &__result);
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream_closeAmiSSLForThread_0(void)
{
	function_result __result = { .has_return_value = false };
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream_warmCurrentTask_0(void)
{
	function_result __result = { .has_return_value = false };
	return __result;
}
