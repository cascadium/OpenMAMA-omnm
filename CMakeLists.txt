cmake_minimum_required (VERSION 2.8.11)
project (OpenMAMA-omnm)

if(UNIX)
else()
    # Note path is relative to where it would be executed
    set(DEFAULT_MAMA_SRC ../../OpenMAMA)
    set(DEFAULT_MAMA_ROOT "D:\\SourceExternal\\fquinner\\OpenMAMA\\openmama_install_6.1.0")
endif()

if(NOT MAMA_SRC)
    set(MAMA_SRC ${DEFAULT_MAMA_SRC})
endif()

if(NOT MAMA_ROOT)
    set(MAMA_ROOT ${DEFAULT_MAMA_ROOT})
endif()

add_subdirectory(src)