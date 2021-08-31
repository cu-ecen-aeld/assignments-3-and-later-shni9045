#!/bin/bash

# Check to see if correct number of script parameters are specified
if [ $# -ne 2 ]
then 
     printf "Not All Or Excess Parameters Specified"
     exit 1
else
    if [ -f $1 ]
    then
        # Update the access and modification time-stamp of file
        touch -c-d  $1
        # Write given string to file
        echo "$2" > $1

    else
        # Just Create an empty file
        touch $1
        # Write given string to file
        echo "$2" > $1
    fi
    # Check for file creation
    if [ ! -f $1 ]
    then 
        echo "File Could Not Be Created"
        exit 1
    fi  
fi