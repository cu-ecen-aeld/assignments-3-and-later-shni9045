#!/bin/bash

#echo "Ok"

minarg=1
numfiles=0


if [ $# -lt 2 ]
then 
     printf "Not All Parameters Specified"
     exit 1
else
    if [ -e $2 ]
    then
        touch -a $1
        for str in $2
        do
        echo "$str"> $1
        done
    else
        
        for str in $2
        do
        echo "$str"> $1
        done
    fi       
fi