# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT all_t.h all_t.c all_args.h
  DEPENDS oeedger8r
          ../edl/aliasing.edl
          ../edl/all.edl
          ../edl/array.edl
          ../edl/basic.edl
          ../edl/deepcopy.edl
          ../edl/enum.edl
          ../edl/errno.edl
          ../edl/foreign.edl
          ../edl/other.edl
          ../edl/pointer.edl
          ../edl/string.edl
          ../edl/struct.edl
          ../edl/switchless.edl
  COMMAND
    oeedger8r --experimental --trusted --search-path
    ${CMAKE_CURRENT_SOURCE_DIR}/../edl --search-path
    ${CMAKE_CURRENT_SOURCE_DIR}/../moreedl all.edl)

add_custom_command(
  OUTPUT bar_t.h bar_args.h
  DEPENDS ../moreedl/bar.edl
  COMMAND oeedger8r --trusted --header-only --search-path
          ${CMAKE_CURRENT_SOURCE_DIR}/../moreedl bar.edl)

add_library(
  oeedger8r_comprehensive_enc SHARED
  all_t.h
  all_t_wrapper.cpp
  bar_t.h
  config.cpp
  foo.cpp
  testaliasing.cpp
  testarray.cpp
  testdeepcopy.cpp
  testenum.cpp
  testforeign.cpp
  testpointer.cpp
  teststruct.cpp
  testswitchless.cpp)

target_link_libraries(oeedger8r_comprehensive_enc oeedger8r_test_enclave)

# The tests intentionally use floats etc in size context. Disable warnings.
if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
  set_source_files_properties(all_t.c PROPERTIES COMPILE_FLAGS
                                                 "-Wno-conversion")
  set_source_files_properties(testpointer.cpp teststring.cpp
                              PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif ()

target_include_directories(
  oeedger8r_comprehensive_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                                      ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (NOT WIN32)
  # Re-enable strict aliasing. TODO: Remove this when #1717 is resolved.
  target_compile_options(oeedger8r_comprehensive_enc
                         PUBLIC -fstrict-aliasing -Werror=strict-aliasing)
endif ()

set_target_properties(oeedger8r_comprehensive_enc PROPERTIES PREFIX "")
