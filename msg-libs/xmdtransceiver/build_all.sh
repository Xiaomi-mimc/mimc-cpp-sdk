#!/bin/sh
set -x

sh build.sh lx01
sh bulid.sh lx04
sh build.sh s12
sh build.sh s12a
sh build.sh s20
sh build.sh clock
sh build.sh x86_64
