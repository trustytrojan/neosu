#!/bin/bash
# Simple script to build neosu in Docker
# NOTE: this is copied from my mcosu-ng repo, and its all kinda shitty and confusing

# Setting up build directory.. (note: i have no idea what this is doing)
cd /build
cp -n -r /src/* ./

echo "Building for target: $HOST"
set -e

# Building..
autoreconf
#./configure --disable-system-deps --enable-static --disable-native --with-audio="bass,soloud" --host=$HOST
./configure --host=$HOST
make -j$(nproc) install

# symlinks turn into copies in .zip files for GHA artifacts, so we need to make a zip of the zip...
zip -r -y -8 bin.zip ./dist/bin-*/
rm -rf dist/*
mv bin.zip dist/

echo "Done!"
