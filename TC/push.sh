#!/bin/sh

. ./config

TC_PATH=/opt/home/$PKG_NAME

echo $TC_PATH

sudo rm -rf ./results/*

sdb shell "mkdir -p $TC_PATH"

sdb push . $TC_PATH


