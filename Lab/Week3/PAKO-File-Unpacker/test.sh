#!/bin/bash
set -x
e=${1?"exe"}
f=`mktemp`
d=`mktemp -d`
s="owo"
wget inp111.zoolab.org/lab02.1/testcase.pak -O $f
$e $f $d/$s
cd $d/$s
chmod +x checker
./checker
rm -fr $f $d
