# mogli
The molecular graph library

## Dependencies

- LEMON 1.3
- Boost 1.60.0

## Compiling

First, get Boost (including `boost-python`) 1.60.0 or newer using your package manager or download and manually install from here: http://www.boost.org/users/download/
If you have an older version of Boost, change line 16 in the mogli `CMakeLists.txt`

    find_package( Boost 1.60.0 REQUIRED COMPONENTS python)
     
to your version (Use older versions at your own risk. In theory they should work fine, but have not been tested).
 
Second, LEMON 1.3 needs to be installed:

    wget http://lemon.cs.elte.hu/pub/sources/lemon-1.3.tar.gz
    tar xvzf lemon-1.3.tar.gz
    cd lemon-1.3
    cmake -DCMAKE_INSTALL_PREFIX=~/lemon
    make install

Note: On Mac OS 10.9, comment out the following two lines and add the code below at line 159 in the LEMON `CMakeLists.txt` before doing make install.

    #ADD_SUBDIRECTORY(demo) 
    #ADD_SUBDIRECTORY(tools)

    if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ " )
    endif()

You can remove the LEMON sources now, i.e., `rm -rf lemon-1.3`.

Now you can compile mogli:
    
    cd /path/to/mogli
    git submodule init
    git submodule update
    mkdir build
    cd build
    cmake ..
    make

In case auto-detection of LEMON fails, do

    cmake -DLIBLEMON_ROOT=/path/to/lemon ..

## Molecules

A simple example of how to work with the molecule class:

    // create a molecule and register its properties
    Molecule mol;
    
    // read molecule from lgf
    std::ifstream ifs("./data/min_1.lgf", std::ifstream::in);
    mol.read_lgf_stream(ifs);
    ifs.close();

    // iterate over all nodes and print the properties
    for (NodeIt n = mol.get_node_iter(); n != lemon::INVALID; ++n) {
        std::cout << mol.get_element(n) << " "
                  << mol.get_id(n) << " "
                  << boost::any_cast<std::string>(mol.get_property(n, "label2")) << " "
                  << boost::any_cast<double>(mol.get_property(n, "partial_charge")) << std::endl;
    }
    
Check if the molecular graph is connected:

    // is the graph connected?
    std::cout << mol.is_connected() << std::endl; 

Iterating over all neighbors of a node:

    // iterate neighbors w of node v
    for (IncEdgeIt e = mol.get_inc_edge_iter(v); e != lemon::INVALID; ++e) {
            Node w = mol.get_opposite_node(v, e);
    }
    

---

# py3-boost branch: Python 3.13 build of ee6872e (Path A, Boost.Python, algorithm unchanged)

Gate G1 build half (ATB `docs/fragments_migration_plan.md` §4). Imports as `libmogli`, as FDB uses it.

## Build
    scripts/build_lemon.sh                       # LEMON 1.3.1 static -fPIC -> ~/.cache/mogli-build/lemon-1.3.1
    scripts/build_boost_python.sh [/usr/bin/python3.13]   # Boost 1.86.0 python313, STATIC -fPIC -> ~/.cache/mogli-build/boost-1.86.0-py3.13
    uv build --wheel -p /usr/bin/python3.13 .    # or: python3.13 -m build --wheel
Both scripts download into `$MOGLI_CACHE/dl` (default `~/.cache/mogli-build`) and verify sha256. CMake finds Boost/LEMON via
`BOOST_ROOT` / `LIBLEMON_ROOT` env vars (or `-D`), else the cache dirs. Submodules (`lib/msgpack-c`) must be initialised;
msgpack-c, nauty and lad are the vendored copies. Flags: Release = `-O3 -DNDEBUG -std=c++11 -fPIC` (C: `-O3 -DNDEBUG -std=c99 -fcommon -fPIC`),
same as the legacy build. libboost_python is linked statically; `ldd` shows only libstdc++/libm/libgcc_s/libc.

## Python 3 changes (src/util/boosting.cpp)
int/str checks via `mogli_is_int`/`mogli_is_str` (`#if PY_MAJOR_VERSION >= 3`: int fitting a C int -> `int`, else `long`; str = unicode);
`__next__` next to `next`; `pack_*`/`hash_*` return `bytes`; `AnyToPython::convert` now raises TypeError instead of
falling off the end (was UB).

## Vector iteration (the return_internal_reference problem)
Kept reference semantics, no `return_by_value` copies of Match:
- `MoleculeVector`/`FragmentVector` hold `shared_ptr<T>`. `return_internal_reference` wrapped the shared_ptr object itself
  (not a `T`), so elements could not be passed to any `T&` method. Now `return_by_value`, which goes through the class's registered
  `shared_ptr<T>` holder: Python receives the *same* C++ object, lifetime shared (no copy).
- `MatchVector` holds `Match` by value; `return_internal_reference<>` works there (custodian keeps the vector alive) and is unchanged
  from the py2 build. The spike's "all three fail" was only true for the shared_ptr vectors.

## Open points for G1
`unpack_*` reverses node order, so re-pack of an unpacked molecule is not byte-identical, and `hash_canonization` depends on input atom
order. Unverified against py2 (no py2 build here): compare in the G1 harness.
