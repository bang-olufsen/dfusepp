# dfusepp

[![build](https://github.com/bang-olufsen/dfusepp/actions/workflows/build.yml/badge.svg)](https://github.com/bang-olufsen/dfusepp/actions/workflows/build.yml)
[![coverage](https://coveralls.io/repos/github/bang-olufsen/dfusepp/badge.svg?branch=main)](https://coveralls.io/github/bang-olufsen/dfusepp?branch=main)
[![lgtm](https://img.shields.io/lgtm/alerts/g/bang-olufsen/dfusepp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/bang-olufsen/dfusepp/alerts/)
[![codefactor](https://www.codefactor.io/repository/github/bang-olufsen/dfusepp/badge)](https://www.codefactor.io/repository/github/bang-olufsen/dfusepp)
[![license](https://img.shields.io/badge/license-MIT_License-blue.svg?style=flat)](LICENSE)

A C++11 header-only library for validating [DfuSe](http://rc.fdr.hu/UM0391.pdf) files on embedded devices.

This library is made for validating DfuSe files without having to save the complete image in RAM. Instead only the offsets to the images are saved for easier extract and copy to e.g. Flash. DfuSe images can be generated using [dfuse-tool](https://github.com/bang-olufsen/dfuse-tool).

By defining `DFUSEPP_IMAGE_ELEMENT_VERSION` it is possible to attach a 4 byte version header (1 byte major, 1 byte minor and 2 bytes patch version) to the image elements. This can be useful if several images are to be included and you want to be able to readout the version and save the additional overhead of the 274 bytes Target Prefix per image. The [dfuse-tool](https://github.com/bang-olufsen/dfuse-tool) also only works with a single DFU image with multiple image elements.

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
        std::vector<Dfusepp::ImageElement> images = dfusepp.images();
        ...
    }
}
```

## Limitations

* Only support a single image with multiple image elements
* Only support little endian targets due to current unions used
