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

if(UNIX)
    set(LIB_WOMBAT_COMMON wombatcommon)
    set(LIB_MAMA_C mama)
    set(LIB_OMNM mamaomnmmsgimpl)
else()
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(MAMA_LIB_SUFFIX "mdd")
    else()
        set(MAMA_LIB_SUFFIX "md")    
    endif()
    set(LIB_WOMBAT_COMMON libwombatcommon${MAMA_LIB_SUFFIX})
    set(LIB_MAMA_C libmamac${MAMA_LIB_SUFFIX})
    set(LIB_OMNM libmamaomnmmsgimpl${MAMA_LIB_SUFFIX})
endif()

add_library(${LIB_OMNM}
            MODULE Field.cpp
                   Iterator.cpp
                   Iterator.h
                   omnmmsgpayloadfunctions.h
                   Payload.cpp
                   Payload.h)

target_link_libraries(${LIB_OMNM}
                      ${LIB_WOMBAT_COMMON}
                      ${LIB_MAMA_C})

if(UNIX)
    set_target_properties(${LIB_OMNM} PROPERTIES PREFIX "")
endif()

install(TARGETS ${LIB_OMNM}
        CONFIGURATIONS Release
        DESTINATION bin/dynamic)

# CMAKE_CL_64 = true / false
install(TARGETS ${LIB_OMNM}
        CONFIGURATIONS Debug
        DESTINATION bin/dynamic-debug)
