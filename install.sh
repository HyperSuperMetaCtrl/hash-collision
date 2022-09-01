#/bin/sh

git clone https://github.com/weidai11/cryptopp.git lib/cryptopp
cp cryptopp-cmake/CMakeLists.txt cryptopp/cmake_install lib/cryptopp

cmake -S . -B build
cd build
make
