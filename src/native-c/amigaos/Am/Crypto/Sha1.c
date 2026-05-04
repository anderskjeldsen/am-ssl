#include <libc/core.h>
#include <Am/Crypto/Sha1.h>
#include <amigaos/Am/Crypto/Sha1.h>
#include <Am/Lang/ClassRef.h>
#include <Am/Lang/UByte.h>
#include <Am/Lang/Array.h>
#include <Am/Lang/Object.h>
#include <libc/core_inline_functions.h>

// AmigaOS m68k SHA-1 binding. Library opening / global library bases
// live in src/native-c/amigaos/amissl_init.{c,h} so every native
// class in am-ssl that uses OpenSSL APIs can share one set of
// strong-symbol globals (defining them per-file would multiply-define
// AmiSSLBase / SocketBase / etc. at link time).

#include <amigaos/amissl_init.h>
#include <openssl/sha.h>

// Lifecycle hooks — Sha1 is a stateless static-only `native class`,
// so all three are no-ops. We deliberately do NOT close AmiSSL
// here — see amissl_init.c for the rationale.

function_result Am_Crypto_Sha1__native_init_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    if (this != NULL) { __increase_reference_count(this); }
__exit: ;
    if (this != NULL) { __decrease_reference_count(this); }
    return __result;
}

function_result Am_Crypto_Sha1__native_release_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    return __result;
}

function_result Am_Crypto_Sha1__native_mark_children_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    return __result;
}

function_result Am_Crypto_Sha1_digest_0(aobject * var_input)
{
    function_result __result = { .has_return_value = true };
    bool __returning = false;
    aobject *result_array;
    array_holder *a_holder;
    array_holder *r_holder;
    unsigned int input_len;
    unsigned char *input_data;
    unsigned char *result_data;
    unsigned char digest[SHA_DIGEST_LENGTH]; // 20 bytes
    int i;

    if (var_input != NULL) { __increase_reference_count(var_input); }

    if (!amissl_ensure_initialised()) {
        // AmiSSL couldn't be opened. Return a NULL array — callers
        // get a length()/index access exception, recoverable rather
        // than freezing the system.
        __result.return_value.flags = 0;
        __result.return_value.value.object_value = NULL;
        goto __exit;
    }

    a_holder = (array_holder *) &var_input[1];
    input_len = a_holder->size;
    input_data = (unsigned char *) a_holder->array_data;

    SHA1(input_data, input_len, digest);

    result_array = __create_array(SHA_DIGEST_LENGTH, 1, &Am_Lang_Array_ta_Am_Lang_UByte, uchar_type);
    r_holder = (array_holder *) &result_array[1];
    result_data = (unsigned char *) r_holder->array_data;
    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        result_data[i] = digest[i];
    }

    __result.return_value.flags = 0;
    __result.return_value.value.object_value = result_array;

__exit: ;
    if (var_input != NULL) { __decrease_reference_count(var_input); }
    return __result;
}
