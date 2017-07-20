#!/bin/bash
cd ../data

keyfile="$PWD/.upload"
#espurl="192.168.2.246/edit"
espurl="esp_Rocker/edit"
#espurl="192.168.5.1/edit"

for file in ../data/*.gz; do
    if [ "$keyfile" -ot "$file" ]; then
        #echo "uploading $file"
        curl -F "file=@$PWD/$file" "$espurl"
    fi
done
touch "$keyfile"
