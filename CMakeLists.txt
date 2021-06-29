project( pcap_reader )
cmake_minimum_required( VERSION 2.8 )

# remove compiler checks if you are using VS2010, it supports
#   1. auto
#   2. lambda
#   3. const iterator
# and, sadly, it doesn't support `for(item, cotainer)'
set( CMAKE_CXX_FLAGS "-g -Wall" )
include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++14"   COMPILER_SUPPORTS_CXX14 )
check_cxx_compiler_flag( "-std=c++0x"   COMPILER_SUPPORTS_CXX0X )
if( COMPILER_SUPPORTS_CXX14 )
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14" )
    endif()
elseif( COMPILER_SUPPORTS_CXX0X)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
else()
    # MSVC, On by default (if available)
endif()

add_executable( ${PROJECT_NAME} src/main.cpp src/csv/writer.cpp
                src/md/preprocessor.cpp src/md/snapshot.cpp
                src/pcap/pcap_reader.cpp)
target_link_libraries( ${PROJECT_NAME} pcap z )
