/*
 * errno shim. With -noixemul we don't pull in libnix's errno via ixemul,
 * so something has to define it for AmiSSL's bsdsocket inlines to link.
 */

int errno = 0;
