# dfusepp

[![build](https://github.com/bang-olufsen/dfusepp/actions/workflows/build.yml/badge.svg)](https://github.com/bang-olufsen/dfusepp/actions/workflows/build.yml)
[![coverage](https://coveralls.io/repos/github/bang-olufsen/dfusepp/badge.svg?branch=main)](https://coveralls.io/github/bang-olufsen/dfusepp?branch=main)
[![license](https://img.shields.io/badge/license-MIT_License-blue.svg?style=flat)](LICENSE)

A C++11 header-only library for validating [DfuSe](http://rc.fdr.hu/UM0391.pdf) files on embedded devices.

This library is made for validating DfuSe files without having to save the complete image in RAM. Instead only the offsets to the images are saved for easier extract and copy to e.g. Flash. DfuSe images can be generated using [dfuse-tool](https://github.com/bang-olufsen/dfuse-tool).

## Usage

Data is added to Dfusepp using the `addData` function. This function can be called multiple times e.g. when reading a file to limit the amount of RAM required. For more usage examples please see the [unit tests](https://github.com/bang-olufsen/dfusepp/blob/main/test/src/TestDfusepp.cpp).

```cpp
#include <array>
#include <vector>
#include <Dfusepp.h>

int main()
{
    Dfusepp::Dfusepp dfusepp;
    std::array<uint8_t, 1024> dfuImage { ... };

    dfusepp.addData(dfuImage.data(), 0, dfuImage.size());
    if (dfusepp.valid()) {
        std::vector<Dfusepp::DfuseppImageElement> images = dfusepp.images();
        ...
    }
}
```
