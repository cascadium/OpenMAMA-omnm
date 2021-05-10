cmake_minimum_required (VERSION 3.11)

project (OpenMAMA-omnm)

include(GoogleTest)
enable_testing()

set (CMAKE_CXX_STANDARD 11)
set (MAMA_ROOT "/opt/openmama" CACHE PATH "Path to OpenMAMA installation")

add_definitions(-DOPENMAMA_INTEGRATION)

if (MSVC)
  set (OPENMAMA_LIBS libmamacmd libwombatcommonmd)
else (NOT MSVC)
  set (OPENMAMA_LIBS mama pthread wombatcommon)
  # enable warnings
  set(WARNFLAGS "${WARNFLAGS} -Wall")
  set(WARNFLAGS "${WARNFLAGS} -Wextra")
  set(WARNFLAGS "${WARNFLAGS} -Wcast-align")
  set(WARNFLAGS "${WARNFLAGS} -Wformat")
  set(WARNFLAGS "${WARNFLAGS} -Wformat-nonliteral")                # warn about non-literal format strings in printf etc.
  #set(WARNFLAGS "${WARNFLAGS} -Wexit-time-destructors")
  # disable warnings
  set(WARNFLAGS "${WARNFLAGS} -Wno-reorder")                       # order of initialization in ctor
  set(WARNFLAGS "${WARNFLAGS} -Wno-unused-parameter")              # given that API is defined in interface, this is kind of hard to enforce
  set(WARNFLAGS "${WARNFLAGS} -Wno-ignored-qualifiers")            # e.g., const on value return types
endif()

option(ENABLE_ASAN "Build with address sanitizer" OFF)
if(ENABLE_ASAN)
  message(STATUS "Instrumenting with Address Sanitizer")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope")
endif()

option(ENABLE_TSAN "Build with thread sanitizer" OFF)
if(ENABLE_TSAN)
  message(STATUS "Instrumenting with Thread Sanitizer")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=thread -fPIE")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=thread -fPIE")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -pie")
endif()

option(ENABLE_UBSAN "Build with undefined behavior sanitizer" OFF)
if(ENABLE_UBSAN)
  message(STATUS "Instrumenting with Undefined Behavior Sanitizer")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fno-omit-frame-pointer")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=undefined")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=implicit-conversion")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=implicit-integer-truncation")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=integer")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=nullability")
  set(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=vptr")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UBSAN_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${UBSAN_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${UBSAN_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNFLAGS}")

add_subdirectory(src)

# We'll need OpenMAMA's source code for unit test code
include(FetchContent)
FetchContent_Declare(
  openmama
  GIT_REPOSITORY https://github.com/finos/OpenMAMA.git
)
FetchContent_Populate(openmama)

# We need gtest for unit tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${MAMA_ROOT}/include)
include_directories(${MAMA_ROOT}/include/wombat)
include_directories(${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c)
include_directories(${openmama_SOURCE_DIR}/mama/c_cpp/src/c)
link_directories(${MAMA_ROOT}/lib)
link_directories(${GTEST_ROOT}/lib)

add_executable(UnitTestMamaMsgC
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/MainUnitTestC.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgatomictests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgcompositetests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldatomictests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldcompositetests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldvectortests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msggeneraltests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgiterationtests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgstatustests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/mamamsg/msgvectortests.cpp
)
target_link_libraries(UnitTestMamaMsgC ${OPENMAMA_LIBS} gtest)
gtest_discover_tests(
    UnitTestMamaMsgC
    EXTRA_ARGS -m qpid -p omnmmsg -i Q
    PROPERTIES ENVIRONMENT
        "WOMBAT_PATH=${openmama_SOURCE_DIR}/mama/c_cpp/src/examples:${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c"
)

add_executable(UnitTestMamaPayloadC
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/MainUnitTestC.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/fieldatomictests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/fieldcompositetests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/fieldvectortests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/payloadatomictests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/payloadcompositetests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/payloadgeneraltests.cpp
    ${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c/payload/payloadvectortests.cpp
)
target_link_libraries(UnitTestMamaPayloadC ${OPENMAMA_LIBS} gtest)
gtest_discover_tests(
    UnitTestMamaPayloadC
    EXTRA_ARGS -m qpid -p omnmmsg -i O
    PROPERTIES ENVIRONMENT
        "WOMBAT_PATH=${openmama_SOURCE_DIR}/mama/c_cpp/src/examples:${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c"
)
