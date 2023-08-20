# require a modern version of cmake.
cmake_minimum_required(VERSION 3.10)

# set the project name and version.
project(libdict VERSION 1.0.0 LANGUAGES C)

# set C standard to C11 without extensions
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# enable verbose compiler warnings.
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# options: should we build static library, shared library, tests, tools?
option(LIBDICT_STATIC "Compile libdict.a" YES)
option(LIBDICT_SHARED "Compile libdict.so" YES)
option(LIBDICT_TOOLS "Compile tools (demo, benchmark, anagram)" YES)
option(LIBDICT_TESTS "Compile tests" YES)

# if we build a shared lib, have the tests and tools link against that,
# otherwise link against static library.
if(LIBDICT_SHARED)
    set(DICT_LIB libdict-shared)
else()
    set(DICT_LIB libdict-static)
endif()

# list of source files.
set(LIBDICT_SOURCES
    src/dict.c
    src/hashtable.c
    src/hashtable2.c
    src/hashtable_common.c
    src/hb_tree.c
    src/pr_tree.c
    src/rb_tree.c
    src/skiplist.c
    src/sp_tree.c
    src/tr_tree.c
    src/tree_common.c
    src/wb_tree.c
)

# list of header files.
set(LIBDICT_HEADERS
    include/dict.h
    include/hashtable.h
    include/hashtable2.h
    include/hb_tree.h
    include/pr_tree.h
    include/rb_tree.h
    include/skiplist.h
    include/sp_tree.h
    include/tr_tree.h
    include/wb_tree.h
)

# adds public include path
include_directories(include)

# adds root project path to module path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# build static library libdict.a
add_library(libdict-static ${LIBDICT_SOURCES} ${LIBDICT_HEADERS})
set_target_properties(libdict-static PROPERTIES
    OUTPUT_NAME dict)

# build shared library libdict.so
add_library(libdict-shared SHARED ${LIBDICT_SOURCES} ${LIBDICT_HEADERS})
set_target_properties(libdict-shared PROPERTIES
    OUTPUT_NAME dict
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# build libdict demo executable (interactive)
add_executable(libdict-demo demo.c)
target_link_libraries(libdict-demo ${DICT_LIB})

# build libdict benchmark executable
add_executable(libdict-benchmark benchmark.c)
target_link_libraries(libdict-benchmark ${DICT_LIB})
target_include_directories(libdict-benchmark PRIVATE src)

# build libdict anagram executable
add_executable(libdict-anagram anagram.c)
target_link_libraries(libdict-anagram ${DICT_LIB})
target_include_directories(libdict-anagram PRIVATE src)

# unless disabled, build unit tests
if(LIBDICT_TESTS)
    find_package(CUnit REQUIRED)
    enable_testing()
    add_executable(libdict-tests unit_tests.c)
    target_include_directories(libdict-tests PRIVATE ${CUNIT_INCLUDE_DIRS})
    target_link_libraries (libdict-tests ${DICT_LIB})
    target_link_libraries(libdict-tests ${CUNIT_LIBRARIES})

    add_test(
        NAME libdict-tests
        COMMAND libdict-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# disable static library building by default if requested
if(NOT LIBDICT_STATIC)
    set_target_properties(libdict-static PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# disable shared library building by default if requested
if(NOT LIBDICT_SHARED)
    set_target_properties(libdict-shared PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# disable tools building by default if not requested
if(NOT LIBDICT_TOOLS)
    set_target_properties(libdict-demo PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(libdict-benchmark PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(libdict-anagram PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# install headers
install(FILES ${LIBDICT_HEADERS}
    DESTINATION include/libdict
    COMPONENT dev)

# install libraries, shared library is part of runtime whereas static library
# is only needed for development
install(TARGETS libdict-shared libdict-static
    LIBRARY DESTINATION lib COMPONENT lib
    ARCHIVE DESTINATION lib COMPONENT dev
    OPTIONAL)

# tools are in separate package
install(TARGETS libdict-demo libdict-benchmark libdict-anagram
    RUNTIME DESTINATION bin COMPONENT tools
    OPTIONAL)

# options for cpack to generate releases
set(CPACK_COMPONENT_TOOLS_DEPENDS "lib")
set(CPACK_COMPONENT_DEV_DEPENDS "lib")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS YES)
set(CPACK_DEBIAN_LIB_PACKAGE_NAME "libdict")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_PACKAGE_CONTACT "Christian Giese <christian@rtbrick.com>")
if (NOT DEFINED LIBDICT_VERSION)
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
else()
    set(CPACK_PACKAGE_VERSION ${LIBDICT_VERSION})
endif()
include(CPack)