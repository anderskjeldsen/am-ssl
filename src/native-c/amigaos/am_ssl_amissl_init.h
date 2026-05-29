#ifndef AM_SSL_AMIGAOS_AMISSL_INIT_H
#define AM_SSL_AMIGAOS_AMISSL_INIT_H

// Shared one-shot OpenSSL-level init for every native class in am-ssl
// that calls OpenSSL APIs on AmigaOS m68k. Library opens are handled
// by `-lamisslauto` (its constructor runs before main); what we add
// here is OPENSSL_init_ssl + RAND_seed, which amisslauto does not do
// itself. See am_ssl_amissl_init.c for the full rationale.
//
// Package-prefixed filename + include guard so the analogous header in
// am-crypto (am_crypto_amissl_init.h) doesn't collide via the shared
// `-I additional/<pkg>/` search path.

// Idempotent — call from every native function that uses an OpenSSL
// API. Returns 1 on success. (Currently always succeeds; the int
// return is kept so consumers can guard a graceful failure path if a
// future implementation needs to.)
int am_ssl_amissl_ensure_initialised(void);

// Per-task AmiSSL bring-up for AmLang Thread workers. AmiSSL tracks
// per-task state (errno location, signal handler, SSL ctx ↔ task
// mapping) keyed by FindTask(NULL). amisslauto's constructor only
// brought up the main task; worker tasks need their own
// OpenLibrary("amisslmaster.library") + OpenAmiSSL() + InitAmiSSL()
// or the TLS handshake stalls when its read/write callback fires from
// a task AmiSSL doesn't know about.
//
// Returns 1 if the current task is now usable for AmiSSL (already was,
// or has just been brought up). Returns 0 only on a real bring-up
// failure. The matching cleanup runs via the Thread finalizer the
// AmLang `SslSocketStream.nativeInit()` registers — which calls back
// into `am_ssl_amissl_close_for_current_task()` below.
int am_ssl_amissl_ensure_initialised_for_current_task(void);

// Tear down the current task's AmiSSL bring-up. Called from the
// Thread finalizer, so `FindTask(NULL)` is still the task that opened.
void am_ssl_amissl_close_for_current_task(void);

// Read the per-task errno slot that AmiSSL writes via the
// AmiSSL_ErrNoPtr we set at InitAmiSSL time. SSL_get_error +
// libc-level `errno` won't tell us socket-layer failures from inside
// AmiSSL; this does. Returns 0 if the current task has no AmiSSL
// bring-up (main task, or worker that never called
// `am_ssl_amissl_ensure_initialised_for_current_task`).
int am_ssl_amissl_current_task_errno(void);

#endif
