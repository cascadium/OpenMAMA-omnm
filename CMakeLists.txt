cmake_minimum_required (VERSION 2.8.7)
project (OpenMAMA-omnm)

# Note path is relative to where it would be executed
set(DEFAULT_MAMA_SRC ../../OpenMAMA)

if(UNIX)
    file(GLOB DEFAULT_MAMA_ROOT ../OpenMAMA/openmama_install_*)
    set(DEFAULT_GTEST_ROOT "/usr/local")
else()
    if(CMAKE_CL_64)
        set(DEFAULT_INSTALL_PREFIX $ENV{ProgramW6432})
    else()
        set(DEFAULT_INSTALL_PREFIX $ENV{PROGRAMFILES})
    endif()

    # Note path is relative to where it would be used
    set(DEFAULT_MAMA_SRC ../../OpenMAMA)
    set(DEFAULT_MAMA_ROOT "${DEFAULT_INSTALL_PREFIX}/OpenMAMA")
endif()

if(NOT MAMA_SRC)
    set(MAMA_SRC ${DEFAULT_MAMA_SRC})
endif()

if(NOT MAMA_ROOT)
    set(MAMA_ROOT ${DEFAULT_MAMA_ROOT})
endif()

if(NOT GTEST_ROOT)
    set(GTEST_ROOT ${DEFAULT_GTEST_ROOT})
endif()

add_subdirectory(src)
