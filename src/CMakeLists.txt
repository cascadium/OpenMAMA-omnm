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
    message(FATAL_ERROR "Windows not supported")
elseif(UNIX)
   target_link_libraries(mamaomnmmsgimpl wombatcommon mama)
   install(TARGETS mamaomnmmsgimpl DESTINATION lib)

   # static linking not supported w/tsan
   if(NOT ENABLE_TSAN)
       # Only build unit tests on linux (AND we're not building for TSAN)
       if (GTEST_ROOT)
          include_directories(${GTEST_ROOT}/include)
          link_directories(${GTEST_ROOT}/lib)
       endif()
       add_executable(unittests UnitTests.cpp)
       target_link_libraries(unittests gtest mamaomnmmsgimpl pthread)
       install(TARGETS unittests DESTINATION bin)
   endif()
endif()

