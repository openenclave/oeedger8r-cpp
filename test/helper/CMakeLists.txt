# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

if (USING_OEEDGER8R_CPP_AS_SUBMODULE)

  set(INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/../../include
                   ${CMAKE_BINARY_DIR}/output/include)

  set(HEADERS "")

else ()

  set(HEADERS
      openenclave/bits/defs.h
      openenclave/bits/eeid.h
      openenclave/bits/exception.h
      openenclave/bits/fs.h
      openenclave/bits/module.h
      openenclave/bits/properties.h
      openenclave/bits/report.h
      openenclave/bits/result.h
      openenclave/bits/sgx/sgxproperties.h
      openenclave/bits/types.h
      openenclave/corelibc/bits/defs.h
      openenclave/corelibc/errno.h
      openenclave/corelibc/limits.h
      openenclave/corelibc/stdlib.h
      openenclave/corelibc/string.h
      openenclave/corelibc/wchar.h
      openenclave/edger8r/common.h
      openenclave/edger8r/enclave.h
      openenclave/edger8r/host.h
      openenclave/enclave.h
      openenclave/host.h
      openenclave/host_verify.h
      openenclave/internal/tests.h)

  foreach (header ${HEADERS})
    set(file
        https://raw.githubusercontent.com/openenclave/openenclave/master/include/${header}
    )
    message("Downloading ${file}")
    file(DOWNLOAD ${file} ${CMAKE_CURRENT_BINARY_DIR}/${header})
  endforeach ()

  set(EDL edl/asym_keys.edl)

  foreach (edl ${EDL})
    set(file
        https://raw.githubusercontent.com/openenclave/openenclave/master/${edl})
    message("Downloading ${file}")
    file(DOWNLOAD ${file} ${CMAKE_CURRENT_BINARY_DIR}/${edl})
  endforeach ()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/openenclave/bits/asym_keys.h
    COMMAND oeedger8r --header-only
            ${CMAKE_CURRENT_BINARY_DIR}/edl/asym_keys.edl
    COMMAND
      cmake -E copy_if_different
      ${CMAKE_CURRENT_BINARY_DIR}/openenclave/bits/asym_keys_args.h
      ${CMAKE_CURRENT_BINARY_DIR}/openenclave/bits/asym_keys.h
    WORKING_DIRECTORY openenclave/bits)

  set(INCLUDE_DIRS PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

  list(APPEND HEADERS openenclave/bits/asym_keys.h)

endif ()

set(COMPILE_FLAGS -fvisibility=hidden -fPIC)

add_library(oeedger8r_test_enclave STATIC enclave.cpp ${HEADERS})

target_include_directories(oeedger8r_test_enclave PUBLIC ${INCLUDE_DIRS})

target_compile_options(oeedger8r_test_enclave PUBLIC ${COMPILE_FLAGS})

add_library(oeedger8r_test_host STATIC host.cpp ${HEADERS})

target_include_directories(oeedger8r_test_host PUBLIC ${INCLUDE_DIRS})

target_compile_options(oeedger8r_test_host PUBLIC ${COMPILE_FLAGS})
