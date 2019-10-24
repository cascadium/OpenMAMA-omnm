cmake_minimum_required (VERSION 2.8.7)
project (OpenMAMA-omnm)

if(UNIX)
    file(GLOB DEFAULT_MAMA_ROOT /opt/openmama)
    set(DEFAULT_GTEST_ROOT "/usr")
else()
    if(CMAKE_CL_64)
        set(DEFAULT_INSTALL_PREFIX $ENV{ProgramW6432})
    else()
        set(DEFAULT_INSTALL_PREFIX $ENV{PROGRAMFILES})
    endif()

    # Note path is relative to where it would be used
    set(DEFAULT_MAMA_ROOT "${DEFAULT_INSTALL_PREFIX}/OpenMAMA")
endif()

if(NOT MAMA_ROOT)
    set(MAMA_ROOT ${DEFAULT_MAMA_ROOT})
endif()

if(NOT GTEST_ROOT)
    set(GTEST_ROOT ${DEFAULT_GTEST_ROOT})
endif()

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
    install(TARGETS UnitTestMamaMsgC DESTINATION bin)

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
    install(TARGETS UnitTestMamaPayloadC DESTINATION bin)
else()
    message(WARNING "If you want full unit tests, you should provide -DOPENMAMA_SRC=")
endif()
