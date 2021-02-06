# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

set(PASS_STRING "oe-nix-shell build completed")

add_test(
  NAME oe-nix-shell-test
  COMMAND
    ${PROJECT_SOURCE_DIR}/nix/oe-nix-shell "rm -rf build && \
	   mkdir build && \
	   cd build && \
	   cmake ${PROJECT_SOURCE_DIR} && \
	   make -j 8 && \
	   echo ${PASS_STRING}")

set_tests_properties(oe-nix-shell-test PROPERTIES PASS_REGULAR_EXPRESSION
                                                  "${PASS_STRING}")
