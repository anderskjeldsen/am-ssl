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
// PLACEHOLDER, BY REQUEST: every entry point is inert. Nothing throws, nothing
// is required to link, and no TLS library is referenced — the point is simply
// to let AROS builds complete while a real implementation is still being
// worked out.
//
// Read reports 0 bytes (which callers see as end-of-stream) and write silently
// discards. That means a TLS connection FAILS QUIETLY rather than erroring:
// deliberate for now, but it is the thing to remember when something that
// should have used https just returns nothing on AROS.
//
// An earlier version threw on read/write so the failure was loud. If you want
// that back while still requiring no libraries, restore __throw_simple_exception
// here — the AmLang side needs no changes either way, and neither does the
// build, since this platform links no SSL libs at all.
//
// Swap this file for a real port once an aarch64-aros OpenSSL (or an AROS
// AmiSSL equivalent) exists.

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
	(void) this; (void) buffer; (void) offset; (void) length;
	// 0 = nothing read; callers treat it as end-of-stream.
	__result.return_value.value.int_value = 0;
	__result.return_value.flags = PRIMITIVE_INT;
	return __result;
}

function_result Am_Net_Ssl_SslSocketStream_write_0(aobject * const this, aobject * buffer, long long offset, unsigned int length)
{
	function_result __result = { .has_return_value = false };
	(void) this; (void) buffer; (void) offset; (void) length;
	// Silently discarded.
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
