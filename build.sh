#!/bin/bash
set -e

mkdir -p .build-external; pushd .build-external
cmake ../external
make -j "$(nproc)"
popd

mkdir -p .build-x86; pushd .build-x86
cmake -DBUILD_EXTERNAL=1 -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake ..;
make -j "$(nproc)"

if [ "$1" = "coverage" ]; then
  lcov -q -c -i -d . -o base.info
  ctest --verbose
  lcov -q -c -d . -o test.info 2>/dev/null
  lcov -q -a base.info -a test.info > total.info
  lcov -q -r total.info "*usr/include/*" "*CMakeFiles*" "*/test/*" "*Catch2*" "*turtle*" -o coverage.info
  genhtml -o coverage coverage.info
  echo "Coverage report can be found in $(pwd)/coverage"
fi

popd
