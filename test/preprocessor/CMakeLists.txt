# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT preprocessor_args.h preprocessor_t.h preprocessor_t.c
  COMMAND oeedger8r -DTEST --trusted ${CMAKE_CURRENT_SOURCE_DIR}/preprocessor.edl)

add_custom_target(
  build_preprocessor_test ALL
  DEPENDS preprocessor_args.h preprocessor_t.h preprocessor_t.c)

# TODO: If the test builds that means at least the preprocessor ran
# and removed the directives. It does not mean that the proper defines
# were passed though. Need to find a way to test this.

