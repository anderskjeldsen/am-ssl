/*
 * Pure-C SHA-1 sanity test for AmigaOS m68k via AmiSSL.
 *
 * Mirrors the AmLang sha1-test example: same inputs, same expected
 * digests, same library link line (-lamisslauto, -lamisslstubs).
 *
 * Purpose is diagnostic — if this program prints the four hashes and
 * exits cleanly on Amiga, but the AmLang sha1-test freezes at exit,
 * then the freeze is something AmLang-runtime-specific (most likely
 * the order of class-shutdown calls vs amisslauto's destructor).
 *
 * If this program also freezes at exit, then -lamisslauto's
 * destructor (CloseAmiSSL + CloseLibrary chain) is the culprit on its
 * own — independent of AmLang. Workaround: stop linking amisslauto
 * and do explicit OpenAmiSSLTags() in user code, never calling
 * CloseAmiSSL on shutdown.
 */

#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

static void hash_and_print(const char *input)
{
    unsigned char digest[SHA_DIGEST_LENGTH]; /* 20 bytes */
    int i;

    SHA1((const unsigned char *)input, strlen(input), digest);

    printf("sha1(\"%s\") = ", input);
    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", (unsigned int)digest[i]);
    }
    printf("\n");
}

int main(void)
{
    /*
     * Diagnostic: comment out the hash calls so amisslauto opens
     * AmiSSL on startup but no SHA1() / OpenSSL function ever runs.
     * If this build hangs at exit while the previous (with the four
     * hash calls) exited cleanly, the bug is in amisslauto's
     * destructor when AmiSSL was opened-but-unused — same shape as
     * the AmLang `"hello".println()`-only test that freezes.
     */
    printf("sha1-c-test starting\n");
#if 0
    hash_and_print("");
    hash_and_print("abc");
    hash_and_print("AmLang");
    hash_and_print("The quick brown fox jumps over the lazy dog");
#endif
    printf("done\n");
    return 0;
}
