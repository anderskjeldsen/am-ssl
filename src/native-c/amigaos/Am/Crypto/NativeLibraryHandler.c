#include <libc/core.h>
#include <Am/Crypto/NativeLibraryHandler.h>
#include <amigaos/Am/Crypto/NativeLibraryHandler.h>
#include <Am/Lang/ClassRef.h>
#include <Am/Lang/Object.h>
#include <libc/core_inline_functions.h>

#include <amigaos/amissl_init.h>

// AmigaOS m68k implementation of the Am.Crypto.NativeLibraryHandler
// singleton. The class itself is just a sentinel — its only job is to
// give us a hook into the AmLang lifecycle:
//
//   * `_native_init_0`   runs when the singleton instance is first
//                        materialised (during class load). We do NOT
//                        open AmiSSL here — opening is lazy in
//                        amissl_init.c so a program that never touches
//                        OpenSSL APIs doesn't pay the cost.
//
//   * `_native_release_0` runs when AmLang's class shutdown sweeps
//                        through and drops the singleton's last
//                        reference, INSIDE main(), before stdio /
//                        memory pools are torn down. This is the
//                        right window to close AmiSSL — earlier than
//                        a -lamisslauto-style GCC destructor (which
//                        runs after main and freezes m68k systems).

function_result Am_Crypto_NativeLibraryHandler__native_init_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    bool __returning = false;
    if (this != NULL) {
        __increase_reference_count(this);
    }
__exit: ;
    if (this != NULL) {
        __decrease_reference_count(this);
    }
    return __result;
}

function_result Am_Crypto_NativeLibraryHandler__native_release_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    bool __returning = false;

    // Only does anything if amissl_ensure_initialised() actually ran
    // and succeeded — see amissl_init.c. Idempotent and safe to call
    // even when AmiSSL was never touched (e.g. a program that uses
    // only Sha1 with no input never reaches digest, or a program that
    // doesn't use this package at all).
    amissl_release_if_initialised();

__exit: ;
    return __result;
}

function_result Am_Crypto_NativeLibraryHandler__native_mark_children_0(aobject * const this)
{
    function_result __result = { .has_return_value = false };
    bool __returning = false;
__exit: ;
    return __result;
}
