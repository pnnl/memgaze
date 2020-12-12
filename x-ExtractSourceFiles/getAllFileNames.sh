#!/bin/bash

case $# in
1) srcFile=$1;;
*) echo "Specify the name of the XML file to be parsed."; exit;;
esac

egrep -e "<(A f)|(F n)=" ${srcFile} | cut -d"\"" -f2 | sort | uniq
