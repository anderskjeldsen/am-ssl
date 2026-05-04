#ifndef AMIGAOS_AMISSL_INIT_H
#define AMIGAOS_AMISSL_INIT_H

#include <exec/types.h>
#include <exec/libraries.h>

// Shared AmiSSL bring-up for every `native class` in am-ssl that calls
// OpenSSL APIs on AmigaOS m68k. Replaces -lamisslauto's automatic
// constructor/destructor pair with explicit lazy init that skips
// cleanup — see amissl_init.c for the rationale.
//
// The four library bases below are referenced by AmiSSL's inline
// headers via baserel addressing. They MUST be globally visible
// strong symbols, defined exactly once across the whole link line.
// Defined in amissl_init.c, externed here for everyone else.

extern struct Library *AmiSSLMasterBase;
extern struct Library *AmiSSLBase;
extern struct Library *AmiSSLExtBase;
extern struct Library *SocketBase;

// Open bsdsocket.library, amisslmaster.library, and AmiSSL itself if
// they aren't already open. Idempotent — call from every native
// function that uses an OpenSSL API.
//
// Returns 1 on success, 0 if any of the libraries couldn't be opened
// (caller should surface this as a graceful error rather than
// crashing/hanging — see Sha1.c for the pattern).
int amissl_ensure_initialised(void);

// Close anything `amissl_ensure_initialised()` opened. Idempotent and
// safe to call when init was never run (no-op in that case). Only
// closes libraries we actually opened ourselves — if libnix's
// -lsocket already had bsdsocket open when we got there, we leave
// SocketBase alone so libnix's own destructor can close it.
//
// Called from `Am_Crypto_NativeLibraryHandler__native_release_0` —
// runs *inside* `main` during AmLang's class shutdown, not from a GCC
// destructor after `main` returns. Doing the close here (rather than
// via -lamisslauto's destructor) sidesteps the m68k shutdown freeze.
void amissl_release_if_initialised(void);

#endif
