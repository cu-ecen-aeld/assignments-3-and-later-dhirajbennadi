#!/bin/sh
# Shell script for Assignment 1
# Author: Dhiraj Bennadi



echo "Executing Shell script: $0"

if [ $# -lt 2 ]
then
    echo "Less number of arguments passed"
    exit 1
fi

writefile=$1
writestr=$2

if [ ! -d "$writefile" ];
then
    echo "Directory $filesdir doesnt exist"
    echo "Creating the Directory"
    echo "${writefile%/*}"

    mkdir -p "${writefile%/*}"
fi


#input="/home/data.txt"
# extract data.txt
file_name="${writefile##*/}"
# get .txt 
file_extension="${writefile##*.}"
# get data 
file="${writefile%.*}"

cd "${writefile%/*}"
#pwd

touch $file_name



if [ ! -f "$file_name" ];
then
    echo "Directory $writefile doesnt exist"
    exit 1
fi

echo "$writestr" > $file_name

cat $file_name

echo "$0 completed"
echo "\n"

