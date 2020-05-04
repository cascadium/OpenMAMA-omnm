include_directories(.)
include_directories(${MAMA_ROOT}/include)

# Default to installing directly to MAMA directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${MAMA_ROOT}" CACHE PATH "default install path" FORCE)
endif()

link_directories(${MAMA_ROOT}/lib)
link_directories(${MAMA_ROOT}/lib/dynamic)
link_directories(${MAMA_ROOT}/lib/dynamic-debug)

add_definitions(-DBRIDGE -DMAMA_DLL -DOPENMAMA_INTEGRATION)

add_library(mamaomnmmsgimpl
            SHARED Field.cpp
                   Iterator.cpp
                   Iterator.h
                   omnmmsgpayloadfunctions.h
                   Payload.cpp
                   Payload.h)

if(WIN32)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(MAMA_LIB_SUFFIX "mdd")
    else()
        set(MAMA_LIB_SUFFIX "md")
    endif()
    target_link_libraries(mamaomnmmsgimpl
                          libwombatcommon${MAMA_LIB_SUFFIX}
                          libmamac${MAMA_LIB_SUFFIX})
    set_target_properties(mamaomnmmsgimpl PROPERTIES PREFIX "lib")

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # Windows Targets
    install(TARGETS mamaomnmmsgimpl
            CONFIGURATIONS Release
            DESTINATION bin/dynamic)
    install(TARGETS mamaomnmmsgimpl
            CONFIGURATIONS Debug
            DESTINATION bin/dynamic-debug)

elseif(UNIX)
    add_library(mamaomnmmsgimpl-static
                STATIC Field.cpp
                       Iterator.cpp
                       Iterator.h
                       omnmmsgpayloadfunctions.h
                       Payload.cpp
                       Payload.h)
   install(TARGETS mamaomnmmsgimpl-static DESTINATION lib)

   target_link_libraries(mamaomnmmsgimpl wombatcommon mama)
   install(TARGETS mamaomnmmsgimpl DESTINATION lib)

   if( (NOT ENABLE_TSAN) AND (GTEST_ROOT) )
       # Only build unit tests on linux (AND we're not building for TSAN)
       # (static linking not supported w/tsan)
       include_directories(${GTEST_ROOT}/include)
       link_directories(${GTEST_ROOT}/lib)
       add_executable(unittests UnitTests.cpp)
       target_link_libraries(unittests gtest mamaomnmmsgimpl pthread)
       install(TARGETS unittests DESTINATION bin)
   endif()
endif()

# Benchmarking utility
add_executable(OmnmBenchmark
        Benchmarker.cpp
        Benchmarker.h
)
target_link_libraries(OmnmBenchmark mamaomnmmsgimpl-static wombatcommon mama mamacpp pthread)