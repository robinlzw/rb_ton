cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

add_executable (validator-engine-console validator-engine-console.cpp
  validator-engine-console.h validator-engine-console-query.cpp
  validator-engine-console-query.h )
target_link_libraries(validator-engine-console tdutils tdactor adnllite tl_api tl_lite_api tl-lite-utils ton_crypto ton_block terminal git)

install(TARGETS validator-engine-console RUNTIME DESTINATION bin)

set(CMAKE_C_COMPILER "/usr/local/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message(FATAL_ERROR "You need to build with Clang for this example to work. "
            "export CC=`which clang` export CXX=`which clang++`")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

set(CMAKE_C_FLAGS_ADDRESSSANITIZER "-g -O1 -fsanitize=address -fno-omit-frame-pointer"
CACHE STRING "Flags used by the C compiler during ADDRESSSANITIZER builds.")
set(CMAKE_CXX_FLAGS_ADDRESSSANITIZER "-g -O1 -fsanitize=address -fno-omit-frame-pointer"
CACHE STRING "Flags used by the CXX compiler during ADDRESSSANITIZER builds.")
set(CMAKE_SHARED_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
CACHE STRING "Flags used by the linker during the creation of shared libraries during ADDRESSSANITIZER builds.")
set(CMAKE_MODULE_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
CACHE STRING "Flags used by the linker during the creation of shared libraries during ADDRESSSANITIZER builds.")
set(CMAKE_EXEC_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
CACHE STRING "Flags used by the linker during ADDRESSSANITIZER builds.")

macro(fuzzer name)
    add_executable(${name} validator-engine-console-fuzz.cpp
      validator-engine-console.h validator-engine-console-query.cpp
      validator-engine-console-query.h)
    set_target_properties(${name}
            PROPERTIES
            COMPILE_FLAGS "-fsanitize=fuzzer"
            LINK_FLAGS "-fsanitize=fuzzer")
    target_link_libraries(${name}
            PRIVATE
            tdutils tdactor adnllite tl_api tl_lite_api tl-lite-utils ton_crypto ton_block terminal git
            )
endmacro()

fuzzer( fuzz_validator_engine_console )