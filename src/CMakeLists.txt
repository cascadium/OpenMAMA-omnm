include_directories(.)
string(TOLOWER ${CMAKE_SYSTEM_NAME} system)
include_directories(${MAMA_ROOT}/include)
include_directories(${MAMA_ROOT}/include/wombat)
include_directories(${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c)
include_directories(${openmama_SOURCE_DIR}/mama/c_cpp/src/c)
include_directories(${openmama_SOURCE_DIR}/common/c_cpp/src/c)
include_directories(${openmama_SOURCE_DIR}/common/c_cpp/src/c/${system})
include_directories(${openmama_BINARY_DIR}/mama/c_cpp/src/c)

enable_testing()

# Default to installing directly to MAMA directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${MAMA_ROOT}" CACHE PATH "default install path" FORCE)
endif()

link_directories(${MAMA_ROOT}/lib)
link_directories(${MAMA_ROOT}/lib/dynamic)
link_directories(${MAMA_ROOT}/lib/dynamic-debug)
link_directories(${openmama_BINARY_DIR}/mama/c_cpp/src/c)
link_directories(${openmama_BINARY_DIR}/mama/c_cpp/src/c/${CMAKE_BUILD_TYPE})
link_directories(${openmama_BINARY_DIR}/common/c_cpp/src/c)
link_directories(${openmama_BINARY_DIR}/common/c_cpp/src/c/${CMAKE_BUILD_TYPE})

add_definitions(-DBRIDGE -DMAMA_DLL -DOPENMAMA_INTEGRATION)

add_library(mamaomnmmsgimpl
            SHARED Field.cpp
                   Iterator.cpp
                   Iterator.h
                   mama/integration/bridge/omnmmsgpayloadfunctions.h
                   mama/integration/bridge/omnmmsgpayloadimpl.h
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
    set_target_properties(mamaomnmmsgimpl PROPERTIES OUTPUT_NAME "mamaomnmmsgimpl${MAMA_LIB_SUFFIX}")

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # Windows Targets
    install(TARGETS mamaomnmmsgimpl DESTINATION bin)
elseif(UNIX)
    add_library(mamaomnmmsgimpl-static
                STATIC Field.cpp
                       Iterator.cpp
                       Iterator.h
                       mama/integration/bridge/omnmmsgpayloadfunctions.h
                       Payload.cpp
                       Payload.h)
    install(TARGETS mamaomnmmsgimpl-static DESTINATION lib)

    target_link_libraries(mamaomnmmsgimpl wombatcommon mama)
    install(TARGETS mamaomnmmsgimpl DESTINATION lib)

    if(WITH_UNITTEST AND (NOT ENABLE_TSAN) )
        # Only build unit tests on linux (AND we're not building for TSAN)
        # (static linking not supported w/tsan)
        include_directories(${GTEST_ROOT}/include)
        link_directories(${GTEST_ROOT}/lib)
        add_executable(unittests_omnm UnitTests.cpp)
        target_link_libraries(unittests_omnm gtest mamaomnmmsgimpl pthread)
        gtest_discover_tests(
            unittests_omnm
            EXTRA_ARGS -m qpid -p omnmmsg -i O
            PROPERTIES ENVIRONMENT
                "WOMBAT_PATH=${openmama_SOURCE_DIR}/mama/c_cpp/src/examples:${openmama_SOURCE_DIR}/mama/c_cpp/src/gunittest/c"
        )
    endif()
endif()

install(FILES mama/integration/bridge/omnmmsgpayloadfunctions.h mama/integration/bridge/omnmmsgpayloadimpl.h
        DESTINATION include/mama/integration/bridge)

# Benchmarking utility
if (NOT MSVC)
    add_executable(OmnmBenchmark
            Benchmarker.cpp
            Benchmarker.h
    )
    target_link_libraries(OmnmBenchmark mamaomnmmsgimpl wombatcommon mama mamacpp pthread)
endif ()
