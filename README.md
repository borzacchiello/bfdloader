# BFDLoader

A wrapper of libbfd. It saves the hassle of compiling and using libbfd as it exposes a single high-level API.

## Install

Only Linux and MacOS are currectly supported.

Using cmake:

``` bash
mkdir build
cd build
cmake ..
make -j`nproc`
sudo make install
```

It will download and compile the binutils, compile this wrapper library, and generate a single static library.

## Usage

``` C
#include <bfdloader.h>

// [...]

LoadedBinary lb;
const char*  path = "/path/to/binary/executable";
if (bfdloader_load(&lb, path) != 0) {
    // handle error...
}

// The `lb` object contains segments, symbols and dynamic relocations...
// [...]

bfdloader_destroy(&lb);

// [...]
```

You can look at [bfdinfo.c](./bfdinfo.c) to view a more complete example.

## Go Bindings

The library has bindings for go https://github.com/borzacchiello/gobfdloader
