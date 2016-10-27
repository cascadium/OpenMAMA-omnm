cmake_minimum_required (VERSION 2.8.11)
project (OpenMAMA-omnm)

# Note path is relative to where it would be executed
set(DEFAULT_MAMA_SRC ../../OpenMAMA)

if(UNIX)
    set(DEFAULT_MAMA_ROOT ../OpenMAMA/openmama_install_*)
else()
    set(DEFAULT_MAMA_ROOT "C:\\Program Files\\OpenMAMA")
endif()

if(NOT MAMA_SRC)
    set(MAMA_SRC ${DEFAULT_MAMA_SRC})
endif()

if(NOT MAMA_ROOT)
    set(MAMA_ROOT ${DEFAULT_MAMA_ROOT})
endif()

add_subdirectory(src)
