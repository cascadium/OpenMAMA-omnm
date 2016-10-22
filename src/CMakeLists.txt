include_directories(.)
include_directories(${MAMA_SRC}/mama/c_cpp/src/c)
include_directories(${MAMA_SRC}/common/c_cpp/src/c)
include_directories(${MAMA_ROOT}/include)

# Default to installing directly to MAMA directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${MAMA_ROOT}" CACHE PATH "default install path" FORCE)
endif()

link_directories(${MAMA_ROOT}/lib)
link_directories(${MAMA_ROOT}/lib/dynamic)
link_directories(${MAMA_ROOT}/lib/dynamic-debug)

add_definitions(-DBRIDGE -DMAMA_DLL)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(MAMA_LIB_SUFFIX "mdd")
else()
    set(MAMA_LIB_SUFFIX "md")    
endif()

add_library(libmamaomnmmsgimpl${MAMA_LIB_SUFFIX}
            MODULE Field.cpp
                   Iterator.cpp
                   Iterator.h
                   omnmmsgpayloadfunctions.h
                   Payload.cpp
                   Payload.h)

target_link_libraries(libmamaomnmmsgimpl${MAMA_LIB_SUFFIX}
                      libwombatcommon${MAMA_LIB_SUFFIX}
                      libmamac${MAMA_LIB_SUFFIX})

install(TARGETS libmamaomnmmsgimpl${MAMA_LIB_SUFFIX}
        CONFIGURATIONS Release
        DESTINATION bin/dynamic)

# CMAKE_CL_64 = true / false
install(TARGETS libmamaomnmmsgimpl${MAMA_LIB_SUFFIX}
        CONFIGURATIONS Debug
        DESTINATION bin/dynamic-debug)
