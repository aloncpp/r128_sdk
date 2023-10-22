#!/bin/sh

find ./ -name *.o -type f -print -exec rm -rf {} \;
find ./ -name *.d -type f -print -exec rm -rf {} \;
