#!/bin/bash

mkdir ../tmp

echo "Populating submodules for stashing"
git submodule update --recursive --init

cp -r ./hw ../tmp/hw
cp -r ./Resources ../tmp/Resources
cp -r ./sw/inc ../tmp/inc

git rm ./hw
git rm ./Resources
git rm ./sw/inc

git add .
git commit -m "Prep Flatten"

echo "removing git markers, and reintroducing files"
find ../tmp -type d -name .git -delete
find ../tmp -name .git -delete

mv ../tmp/hw ./hw
mv ../tmp/Resources ./Resources
mv ../tmp/inc ./sw/inc

rm -rf ../tmp/*

git add .
git commit -m "Flatten"
