cmake_minimum_required (VERSION 2.8.7)
project (OpenMAMA-omnm)

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

# Only build the extra unit tests if OPENMAMA_SRC is provided
if(OPENMAMA_SRC)
    include_directories(${MAMA_ROOT}/include)
    include_directories(${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c)
    include_directories(${OPENMAMA_SRC}/mama/c_cpp/src/c)
    link_directories(${MAMA_ROOT}/lib)
    link_directories(${GTEST_ROOT}/lib)

    add_executable(UnitTestMamaMsgC
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/MainUnitTestC.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgatomictests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgcompositetests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldatomictests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldcompositetests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgfieldvectortests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msggeneraltests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgiterationtests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgstatustests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/mamamsg/msgvectortests.cpp
    )
    target_link_libraries(UnitTestMamaMsgC mama gtest pthread wombatcommon)

    add_executable(UnitTestMamaPayloadC
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/MainUnitTestC.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/fieldatomictests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/fieldcompositetests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/fieldvectortests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/payloadatomictests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/payloadcompositetests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/payloadgeneraltests.cpp
        ${OPENMAMA_SRC}/mama/c_cpp/src/gunittest/c/payload/payloadvectortests.cpp
    )
    target_link_libraries(UnitTestMamaPayloadC mama gtest pthread wombatcommon)
else()
    message(WARNING "If you want full unit tests, you should provide -DOPENMAMA_SRC=")
endif()
