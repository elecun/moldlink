#!/bin/bash

if [ -z "$1" ]
  then
    echo "No Arguments (Target Machine IPv4 Address)"
    echo "Usage : ./copy.sh <IP x.x.x.x> <ID> <Remote Path>"
    exit 1
fi

#copy execution file to the remote
echo "copying files to target machine"
sshpass -p 'rhemdrltnf' scp -p -r ./bin/moldlink $2@"$1":$3
echo "copied"