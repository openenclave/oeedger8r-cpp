# oeedger8r-cpp
An implementation of oeedger8r in C++

# Prerequisites

## Compiler
- cmake 3.1 or later
- C++ compiler with `C++ 11` support.

The default C++ compilers and cmake that come with Ubuntu 16.04, Ubutnu 18.04 ought to be sufficient for compiling
oeedger8r.
On Windows, VS 2017 Build Tools or later should be sufficient.

## Platforms
The following platforms are tested in CI
- Ubuntu 16.04
- Ubuntu 18.04
- Windows Server 2019

oeedger8r should build and run successfully on most Linux distrubitions and flavors of Windows.

## Building

Clone the repository:
```bash
git clone https://github.com/openenclave/oeedger8r-cpp
```

Create a build folder and configure:
```bash
cd oeedger8r-cpp
mkdir build
cd build
cmake ..
```

This should produce output similar to:
```bash
-- The C compiler identification is GNU 7.5.0
-- The CXX compiler identification is GNU 7.5.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: oeedger8r-cpp/build
```

Build the project
```bash
make -j 4
```

This would generate oeeger8r executable in the build folder:
```bash
ls -la oeedger8r
-rwxr-xr-x 1 user user 379848 Jun  1 14:15 oeedger8r*
```

## Running tests

Use ctest to run all the tests:
```bash
ctest
Test project oeedger8r-cpp/build
      Start  1: oeedger8r_test_basic
 1/19 Test  #1: oeedger8r_test_basic ...........................................   Passed    0.00 sec
      Start  2: oeedger8r_private_trusted_warning
 2/19 Test  #2: oeedger8r_private_trusted_warning ..............................   Passed    0.01 sec
      Start  3: oeedger8r_allow_list_warning
 3/19 Test  #3: oeedger8r_allow_list_warning ...................................   Passed    0.01 sec
      Start  4: oeedger8r_portability_trusted_wchar_t_warning
 4/19 Test  #4: oeedger8r_portability_trusted_wchar_t_warning ..................   Passed    0.01 sec
      Start  5: oeedger8r_portability_trusted_long_double_warning
 5/19 Test  #5: oeedger8r_portability_trusted_long_double_warning ..............   Passed    0.01 sec
      Start  6: oeedger8r_portability_trusted_long_warning
 6/19 Test  #6: oeedger8r_portability_trusted_long_warning .....................   Passed    0.00 sec
      Start  7: oeedger8r_portability_trusted_unsigned_long_warning
 7/19 Test  #7: oeedger8r_portability_trusted_unsigned_long_warning ............   Passed    0.00 sec
      Start  8: oeedger8r_portability_untrusted_wchar_t_warning
 8/19 Test  #8: oeedger8r_portability_untrusted_wchar_t_warning ................   Passed    0.00 sec
      Start  9: oeedger8r_portability_untrusted_long_double_warning
 9/19 Test  #9: oeedger8r_portability_untrusted_long_double_warning ............   Passed    0.00 sec
      Start 10: oeedger8r_portability_untrusted_long_warning
10/19 Test #10: oeedger8r_portability_untrusted_long_warning ...................   Passed    0.00 sec
      Start 11: oeedger8r_portability_untrusted_unsigned_long_double_warning
11/19 Test #11: oeedger8r_portability_untrusted_unsigned_long_double_warning ...   Passed    0.00 sec
      Start 12: oeedger8r_size_signedness_warning
12/19 Test #12: oeedger8r_size_signedness_warning ..............................   Passed    0.01 sec
      Start 13: oeedger8r_count_signedness_warning
13/19 Test #13: oeedger8r_count_signedness_warning .............................   Passed    0.01 sec
      Start 14: oeedger8r_size_and_count_warning
14/19 Test #14: oeedger8r_size_and_count_warning ...............................   Passed    0.01 sec
      Start 15: oeedger8r_deepcopy_value_warning
15/19 Test #15: oeedger8r_deepcopy_value_warning ...............................   Passed    0.00 sec
      Start 16: oeedger8r_missing_trusted_dir
16/19 Test #16: oeedger8r_missing_trusted_dir ..................................   Passed    0.00 sec
      Start 17: oeedger8r_missing_untrusted_dir
17/19 Test #17: oeedger8r_missing_untrusted_dir ................................   Passed    0.00 sec
      Start 18: oeedger8r_missing_search_path
18/19 Test #18: oeedger8r_missing_search_path ..................................   Passed    0.00 sec
      Start 19: oeedger8r_comprehensive
19/19 Test #19: oeedger8r_comprehensive ........................................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 19

Total Test time (real) =   0.10 sec
```
