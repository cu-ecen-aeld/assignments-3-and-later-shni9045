#!/bin/bash

function processfile() {

    for f in $1/*
        do
        if [ -e $f ]
        then

            ((numfiles=numfiles+1))

            match=$(findstr  $2  $f)
            #echo "Recursive Call"
            #echo $f
            #echo $match
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


#echo "Ok"

minarg=1
numfiles=0
matchlines=0
match=3

if [ $# -lt 1 ]
then 
     printf "Not All Parameters Specified"
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
            #echo $file
            #echo $match
            ((matchlines=matchlines+match))

        elif [ -d $file ]
        then
            #echo "Here directory"
            processfile $file $2
             
        fi
        done
    else
        printf "First argument does not represent a directory"
        exit 1
    fi
    #printf "Total Files : %d" ${numfiles}
    #printf "Matching Lines : %d" ${matchlines}
    printf "The number of files are %d and the number of matching lines are %d" ${numfiles} ${matchlines}
fi