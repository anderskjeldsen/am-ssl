#include <libc/core.h>
#include <Am/Crypto/Sha1.h>
#include <Am/Lang/UByte.h>
#include <Am/Lang/Array.h>
#include <libc/core_inline_functions.h>

#include <openssl/sha.h>

// Lifecycle hooks — Sha1 is stateless, but every `native class` needs
// these symbols defined so the runtime's class table can dispatch them.

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
    if (var_input != NULL) { __increase_reference_count(var_input); }

    array_holder *a_holder = (array_holder *) &var_input[1];
    unsigned int input_len = a_holder->size;
    unsigned char *input_data = (unsigned char *) a_holder->array_data;

    unsigned char digest[SHA_DIGEST_LENGTH]; // 20 bytes
    SHA1(input_data, input_len, digest);

    aobject *result_array = __create_array(SHA_DIGEST_LENGTH, 1, &Am_Lang_Array_ta_Am_Lang_UByte, uchar_type);
    array_holder *r_holder = (array_holder *) &result_array[1];
    unsigned char *result_data = (unsigned char *) r_holder->array_data;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        result_data[i] = digest[i];
    }

    __result.return_value.flags = 0;
    __result.return_value.value.object_value = result_array;

__exit: ;
    if (var_input != NULL) { __decrease_reference_count(var_input); }
    return __result;
}
