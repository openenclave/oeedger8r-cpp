# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (WIN32)
  set(COMPILE_FLAGS "")
else ()
  set(COMPILE_FLAGS -fvisibility=hidden -fPIC)
endif ()

add_library(oeedger8r_test_enclave STATIC enclave.cpp)

target_include_directories(oeedger8r_test_enclave PUBLIC ${INCLUDE_DIRS})

target_compile_options(oeedger8r_test_enclave PUBLIC ${COMPILE_FLAGS})

add_library(oeedger8r_test_host STATIC host.cpp)

target_include_directories(oeedger8r_test_host PUBLIC ${INCLUDE_DIRS})

target_compile_options(oeedger8r_test_host PUBLIC ${COMPILE_FLAGS})

if (UNIX)
  target_link_libraries(oeedger8r_test_host PUBLIC -ldl)
endif ()
