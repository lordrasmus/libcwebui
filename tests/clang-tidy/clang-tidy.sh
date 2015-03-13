#!/bin/bash

cd "$(dirname "$0")"

DISABLED=""
DISABLED+=-cert-err34-c,
DISABLED+=-readability-identifier-naming,
DISABLED+=-llvm-header-guard,
DISABLED+=-llvm-include-order,
DISABLED+=-readability-avoid-const-params-in-decls,
DISABLED+=-hicpp-signed-bitwise,
DISABLED+=-readability-else-after-return,
DISABLED+=-clang-analyzer-core.NonNullParamChecker,
DISABLED+=-clang-analyzer-cplusplus.NewDeleteLeaks,
DISABLED+=-clang-analyzer-deadcode.DeadStores,
DISABLED+=-clang-analyzer-security.insecureAPI.strcpy,
DISABLED+=-clang-analyzer-valist.Uninitialized,
DISABLED+=-clang-diagnostic-implicit-function-declaration,
DISABLED+=-clang-analyzer-optin.performance.Padding,
DISABLED+=-misc-macro-parentheses,
DISABLED+=-misc-unused-parameters,
DISABLED+=-misc-misplaced-widening-cast,

DISABLED+=-readability-redundant-declaration,


#DISABLED+=-clang-analyzer-optin.performance.Padding, ab wann ergibt das einen fehler ?


clang-tidy -checks="*,$DISABLED" \
	../../lib/src/*.c -- -DLINUX \
	-I . -I ../../lib/include/ -I ../../lib/include/intern/ -I ../../lib/platform/include/ \
	-I ../../lib/third_party/src/rb_tree/ -I ../../lib/third_party/src/is_utf8/ \
	-I ../../lib/third_party/src/miniz/
