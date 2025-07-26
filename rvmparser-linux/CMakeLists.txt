cmake_minimum_required(VERSION 3.16)

project(rvmparser-crossplatform
    VERSION 1.0.0
    DESCRIPTION "AVEVA PDMS RVM file parser - Cross Platform"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard for libtess2
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -DNDEBUG")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/libs/rapidjson/include
    ${CMAKE_SOURCE_DIR}/libs/libtess2/Include
    ${CMAKE_SOURCE_DIR}/src
)

# Required ThirdParty dependencies
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/DataAccess/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipUtils/include)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipLib/include)

# Define required macros
add_definitions(-DHAVE_EXPORT_EWC=1)
add_definitions(-DHAVE_DATA_ACCESS=1)
add_definitions(-DHAVE_STUDIO_MESH_SERIALIZE=1)
add_definitions(-DHAVE_EXPORT_NAMED_PIPE=1)

message(STATUS "Enabled required ThirdParty libraries")
message(STATUS "DataAccess: ${CMAKE_SOURCE_DIR}/ThirdParty/DataAccess/include")
message(STATUS "E5DZipUtils: ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipUtils/include")
message(STATUS "E5DZipLib: ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipLib/include")

# Source files for rvmparser
file(GLOB RVMPARSER_SOURCES
    "src/*.cpp"
)

# Source files for libtess2
file(GLOB LIBTESS2_SOURCES
    "libs/libtess2/Source/*.c"
)

# Create executable
add_executable(rvmparser
    ${RVMPARSER_SOURCES}
    ${LIBTESS2_SOURCES}
)

# Link ThirdParty libraries if available
if(WIN32)
    # Windows library paths - link libraries directly with correct names
    if(EXISTS ${CMAKE_SOURCE_DIR}/ThirdParty/DataAccess/lib/Release)
        target_link_libraries(rvmparser ${CMAKE_SOURCE_DIR}/ThirdParty/DataAccess/lib/Release/E5DDataAccessLib.lib)
        message(STATUS "Linked DataAccess library: E5DDataAccessLib.lib")
    endif()
    
    if(EXISTS ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipUtils/lib/Release)
        target_link_libraries(rvmparser 
            ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipUtils/lib/Release/E5DZipUtils.lib
            ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipUtils/lib/Release/zstd_static.lib
        )
        message(STATUS "Linked E5DZipUtils libraries: E5DZipUtils.lib, zstd_static.lib")
    endif()
    
    if(EXISTS ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipLib/lib/Release)
        target_link_libraries(rvmparser ${CMAKE_SOURCE_DIR}/ThirdParty/E5DZipLib/lib/Release/E5DZipLib.lib)
        message(STATUS "Linked E5DZipLib library: E5DZipLib.lib")
    endif()
endif()

# Platform-specific settings
if(WIN32)
    # Windows specific settings
    target_compile_definitions(rvmparser PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    if(MSVC)
        target_compile_options(rvmparser PRIVATE /W3)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux specific settings
    target_compile_definitions(rvmparser PRIVATE LINUX)
    
    # Link math library if needed
    target_link_libraries(rvmparser m)
    
    # Set RPATH for shared libraries
    set_target_properties(rvmparser PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
elseif(APPLE)
    # macOS specific settings
    target_compile_definitions(rvmparser PRIVATE MACOS)
    target_link_libraries(rvmparser m)
endif()

# Installation
install(TARGETS rvmparser
    RUNTIME DESTINATION bin
)

# Optional: Create a package
set(CPACK_PACKAGE_NAME "rvmparser-crossplatform")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

# Platform-specific package generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "rvmparser")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;DragNDrop")
endif()

include(CPack)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")