/*
 * Provides the `errno` symbol AmiSSL's bsdsocket/OpenSSL inlines
 * reference. Compiling with -noixemul means we don't pull libnix's
 * errno in via ixemul, so the linker complains unless we define it
 * ourselves. Same shim am-ssl uses for the AmLang amigaos builds
 * (src/native-c/amigaos/errno.c).
 */

int errno = 0;
