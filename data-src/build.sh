#!/bin/bash

build_file()
{
    file=$1
    tfile="../data/$file.gz"
    if [[ "$file" =~ ^mock.* ]]; then
        return
    else
        if [ "$tfile" -ot "$file" ]; then
            echo "zipping $file"
            gzip -c "$file" > "$tfile"
        fi
    fi
}

if [ ! -d "../data" ]; then
    echo "creating data directory"
    mkdir ../data
fi

if [[ $# == 1 && "$1" == "clean" ]]; then
    if [[ -d "../data" ]]; then
        rm -rf ../data
    fi
elif [[ $# == 1 && -f "$1" ]] ; then
    for file in "$@"; do
        build_file "$file"
    done
else
    for file in *.{js,html,css}; do
        build_file "$file"
    done
fi

if [[ $# == 1 && "$1" == "all" ]]; then
    ./upload.sh
fi
