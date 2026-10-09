cmake_minimum_required(VERSION 3.18)
project(mogli)

# LEMON: -DLIBLEMON_ROOT, or env LIBLEMON_ROOT / MOGLI_CACHE (scripts/build_lemon.sh output)
if(DEFINED ENV{LIBLEMON_ROOT})
    set( _lemon_default "$ENV{LIBLEMON_ROOT}" )
elseif(DEFINED ENV{MOGLI_CACHE})
    set( _lemon_default "$ENV{MOGLI_CACHE}/lemon-1.3.1" )
else()
    set( _lemon_default "$ENV{HOME}/.cache/mogli-build/lemon-1.3.1" )
endif()
set( LIBLEMON_ROOT "${_lemon_default}" CACHE PATH "LEMON install prefix (libemon.a, include/lemon)" )
option( MOGLI_BUILD_REPORT "Also build the C++ 'report' executable" OFF )

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -O0 -DDEBUG -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -std=c++11")

set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0 -DDEBUG -std=c99 -fcommon")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -std=c99 -fcommon")

find_package( Python3 REQUIRED COMPONENTS Development.Module )

# Boost.Python, static, built by scripts/build_boost_python.sh for this Python.
# -DBOOST_ROOT=... or env BOOST_ROOT; else the script's default cache dir.
set( BOOST_PYTHON_COMPONENT "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}" CACHE STRING
     "Boost.Python component name (python313, python39, ...)" )
if(NOT BOOST_ROOT AND DEFINED ENV{BOOST_ROOT})
    set( BOOST_ROOT "$ENV{BOOST_ROOT}" )
endif()
if(NOT BOOST_ROOT)
    file(GLOB _boost_cands "$ENV{HOME}/.cache/mogli-build/boost-*-py${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
    if(_boost_cands)
        list(GET _boost_cands -1 BOOST_ROOT)
    endif()
endif()
if(BOOST_ROOT)
    set( Boost_NO_SYSTEM_PATHS ON )
endif()
option( MOGLI_STATIC_BOOST "Link Boost.Python statically" ON )
set( Boost_USE_STATIC_LIBS ${MOGLI_STATIC_BOOST} )
set( Boost_USE_MULTITHREADED ON )
set( Boost_USE_STATIC_RUNTIME OFF )
set( Boost_NO_BOOST_CMAKE ON )
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
find_package( Boost 1.60.0 REQUIRED COMPONENTS ${BOOST_PYTHON_COMPONENT} )
include_directories("${Boost_INCLUDE_DIRS}")
# Boost 1.86 headers probe <stdfloat>, which gcc 11 lacks
add_compile_definitions(BOOST_NO_CXX23_HDR_STDFLOAT)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ " )
endif()

set( CommonLibs
        emon
        m
)

set(HEADER_FILES
        include/canonization.h
        include/molecule.h
        include/periodictable.h
        include/product.h
        include/bronkerbosch.h
        include/orbits.h
        include/fragment.h
        include/subgraph_isomorphism.h
        include/types.h
        include/util/packing.h
        include/mcf.h
        include/match.h
        include/fcanonization.h)

set(SOURCE_FILES
        src/canonization.cpp
        src/molecule.cpp
        src/product.cpp
        src/bronkerbosch.cpp
        src/orbits.cpp
        src/subgraph_isomorphism.cpp
        src/mcf.cpp)

set(NAUTY_SRC
        lib/nauty/nauty.c
        lib/nauty/nautil.c
        lib/nauty/naugraph.c
        lib/nauty/schreier.c
        lib/nauty/naurng.c
        lib/nauty/naugroup.c)

set(LAD_SRC
        lib/lad/allDiff.c
        lib/lad/domains.c
        lib/lad/graph.c
        lib/lad/lad.c
        lib/lad/sublad.c
        lib/lad/sublad.h
        lib/lad/compatibility.c)

include_directories(
        "include"
        "${LIBLEMON_ROOT}/include"
        "lib/nauty"
        "lib/lad"
        "lib/msgpack-c/include"
)
link_directories( "${LIBLEMON_ROOT}/lib")

add_library(mogli SHARED ${HEADER_FILES} ${SOURCE_FILES} src/util/boosting.cpp ${NAUTY_SRC} ${LAD_SRC})
# Python3::Module: headers only, no libpython (extension modules must not link it)
target_link_libraries(mogli PRIVATE Python3::Module ${Boost_LIBRARIES} ${CommonLibs})
# the module is imported as `libmogli` (BOOST_PYTHON_MODULE(libmogli))
set_target_properties(mogli PROPERTIES PREFIX "lib" OUTPUT_NAME mogli)
# libstdc++/libgcc are the only non-libc runtime deps; keep them dynamic (manylinux-compatible)
install(TARGETS mogli LIBRARY DESTINATION .)

if(MOGLI_BUILD_REPORT)
    add_executable(report test/report.cpp ${SOURCE_FILES} ${NAUTY_SRC} ${LAD_SRC})
    target_link_libraries(report ${Boost_LIBRARIES} ${CommonLibs})
endif()
