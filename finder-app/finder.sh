#!/bin/bash

function processfile() {

    for f in $1/*
        do
        if [ -f $f ]
        then

            ((numfiles=numfiles+1))

            match=$(findstr  $2  $f)

            ((matchlines=matchlines+match))

        elif [ -d $f ]
        then

            processfile $f $2      

        fi
        done

}

function findstr() {

     grep -c "$1" $2

}


minarg=1
numfiles=0
matchlines=0
match=0

if [ $# -ne 2 ]
then 
     printf "Not All Parameters Specified for the script"
     exit 1
else
    if [ -d $1 ]
    then 
        for file in $1/*
        do
        if [ -f $file ]
        then

            ((numfiles=numfiles+1))

            match=$(findstr  $2  $file)

            ((matchlines=matchlines+match))

        elif [ -d $file ]
        then
            processfile $file $2
             
        fi
        done
    else
        printf "First argument does not represent a directory on the filesystem"
        exit 1
    fi

    printf "The number of files are %d and the number of matching lines are %d" ${numfiles} ${matchlines}
fi