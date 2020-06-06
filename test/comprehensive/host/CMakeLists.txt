# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT all_u.h all_u.c all_args.h
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
    oeedger8r --experimental --untrusted --search-path
    ${CMAKE_CURRENT_SOURCE_DIR}/../edl --search-path
    ${CMAKE_CURRENT_SOURCE_DIR}/../moreedl all.edl)

add_custom_command(
  OUTPUT bar_u.h bar_args.h
  DEPENDS ../moreedl/bar.edl
  COMMAND oeedger8r --untrusted --header-only --search-path
          ${CMAKE_CURRENT_SOURCE_DIR}/../moreedl bar.edl)

add_executable(
  oeedger8r_comprehensive_host
  all_u.h
  all_u_wrapper.cpp
  bar_u.h
  main.cpp
  bar.cpp
  foo.cpp
  testarray.cpp
  testdeepcopy.cpp
  testenum.cpp
  testforeign.cpp
  testpointer.cpp
  teststruct.cpp
  testswitchless.cpp)

# The tests intentionally use floats etc in size context. Disable warnings.
if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
  set_source_files_properties(all_u.c PROPERTIES COMPILE_FLAGS
                                                 "-Wno-conversion")
  set_source_files_properties(testpointer.cpp teststring.cpp
                              PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif ()

target_include_directories(
  oeedger8r_comprehensive_host PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                      ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (NOT WIN32)
  # Re-enable strict aliasing. TODO: Remove this when #1717 is resolved.
  target_compile_options(oeedger8r_comprehensive_host
                         PUBLIC -fstrict-aliasing -Werror=strict-aliasing)
endif ()

target_link_libraries(oeedger8r_comprehensive_host oeedger8r_test_host)
