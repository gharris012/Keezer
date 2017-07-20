#!/bin/bash

if [ ! -d "../data" ]; then
    echo "creating data directory"
    mkdir ../data
fi

for file in *.{js,html,css}; do
    tfile="../data/$file.gz"
    if [ "$tfile" -ot "$file" ]; then
        echo "zipping $file"
        gzip -c "$file" > "$tfile"
    fi
done

if [[ $# -eq 1 && $1 -eq "all" ]]; then
    ./upload.sh
fi
