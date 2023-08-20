# libdict

[![Test](https://github.com/rtbrick/libdict/actions/workflows/test.yml/badge.svg?branch=master)](https://github.com/rtbrick/libdict/actions/workflows/test.yml)
[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/rtbrick/libdict?color=green)](https://github.com/rtbrick/libdict/releases)
[![Doxygen documentation](https://img.shields.io/badge/docs-doxygen-green)](https://rtbrick.github.io/libdict/html)
[![GitHub](https://img.shields.io/github/license/rtbrick/libdict)](LICENSE)

libdict is a C library that provides the following data structures with efficient insert, lookup, and delete routines:

* [height-balanced (AVL) tree](http://en.wikipedia.org/wiki/AVL_tree)
* [red-black tree](http://en.wikipedia.org/wiki/Red-black_tree)
* [splay tree](http://en.wikipedia.org/wiki/Splay_tree)
* [weight-balanced tree](https://en.wikipedia.org/wiki/Weight-balanced_tree)
* [path-reduction tree](https://cs.uwaterloo.ca/research/tr/1982/CS-82-07.pdf)
* [treap](http://en.wikipedia.org/wiki/Treap)
* [hashtable using separate chaining](http://en.wikipedia.org/wiki/Hashtable#Separate_chaining)
* [hashtable using open addressing with linear probing](http://en.wikipedia.org/wiki/Hashtable#Open_addressing)
* [skip list](https://en.wikipedia.org/wiki/Skip_list)

All data structures in this library support insert, search, and remove, and have bidirectional iterators. The sorted data structures (everything but hash tables) support near-search operations: searching for the key greater or equal to, strictly greater than, lesser or equal to, or strictly less than, a given key. The tree data structures also support the selecting the nth element; this takes linear time, except in path-reduction and weight-balanced trees, where it only takes logarithmic time.

The API and code are written with efficiency as a primary concern. For example, an insert call returns a boolean indicating whether or not the key was already present in the dictionary (i.e. whether there was an insertion or a collision), and a pointer to the location of the associated data. Thus, an insert-or-update operation can be supported with a single traversal of the data structure. In addition, almost all recursive algorithms have been rewritten to use iteration instead.

Documentation is generated by Doxygen on every commit to master and is available [here](https://rtbrick.github.io/libdict/html).

## Installing

On every release, Debian packages are automatically built. You can download the
latest packages from [here](https://github.com/rtbrick/libdict/releases/latest/download/libdict-debian.zip).

To install, run this:

    wget https://github.com/rtbrick/libdict/releases/latest/download/libdict-debian.zip
    unzip libdict-debian.zip
    sudo apt install -y ./*.deb

This will install the library, along with headers and tools.

## Building

Both GNU Make and CMake are supported. To use the CMake build system, ensure that you have
a recent version of CMake installed (version 3.10 or greater). Then, from within this
repository, run:

    mkdir build
    cd build
    cmake ..
    make -j16

If you want to run unit tests, you can run:

    make test

If you want to generate Debian packages, you can run:

    cpack -G DEB

To build with the included GNU Makefile, simply run `make` in the repository.

## License

libdict is released under the simplified BSD [license](LICENSE). 
