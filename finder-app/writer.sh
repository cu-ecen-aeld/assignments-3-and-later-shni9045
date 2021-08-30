#!/bin/bash

#echo "Ok"

minarg=1
numfiles=0


if [ $# -ne 2 ]
then 
     printf "Not All Or Excess Parameters Specified"
     exit 1
else
    if [ -f $2 ]
    then
        touch -a $1
        echo "$2" > $1

    else
        echo "$2" > $1
    fi       
fi