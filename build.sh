#!/bin/bash

function build_libs() {
	make -C lib
}

function test_libs() {
	make -C lib/libsusanow/tests all
	if [ ! $? = "0" ]; then
		exit -1
	fi
}


build_libs
test_libs
echo "Success Build SUSANOW"

