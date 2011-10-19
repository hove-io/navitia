#!/bin/sh
current_dir=`pwd`
temp=`dirname "$current_dir/$0"`
script_dir=`readlink -f $temp`
source_dir=`readlink -f $script_dir/../source`
fixtures_dir=`readlink -f $script_dir/../fixtures`
mkdir -p $fixtures_dir/navimake
cp -R $source_dir/naviMake/tests/fixture/* $fixtures_dir/navimake
touch $fixtures_dir/test
