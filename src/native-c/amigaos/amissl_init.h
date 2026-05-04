#ifndef AMIGAOS_AMISSL_INIT_H
#define AMIGAOS_AMISSL_INIT_H

// Shared one-shot OpenSSL-level init for every native class in am-ssl
// that calls OpenSSL APIs on AmigaOS m68k. Library opens are handled
// by `-lamisslauto` (its constructor runs before main); what we add
// here is OPENSSL_init_ssl + RAND_seed, which amisslauto does not do
// itself. See amissl_init.c for the full rationale.

// Idempotent — call from every native function that uses an OpenSSL
// API. Returns 1 on success. (Currently always succeeds; the int
// return is kept so consumers can guard a graceful failure path if a
// future implementation needs to.)
int amissl_ensure_initialised(void);

#endif
