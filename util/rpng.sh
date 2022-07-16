#!/bin/sh

test -z "${IMGDIR}" && {
    echo '$IMGDIR is undefined...'
    echo 'try export IMGDIR=/directory/with/images'
    exit 1
}

chunk="${1:-tEXt}"

echo "Searching for png with ${chunk} chunk"

find "$IMGDIR" -name '*.png' -type f -print0 |
    xargs -r0 grep -Zal "$chunk" | shuf -zn 1 |
    xargs -r0I {} cp -v {} ./rand_"${chunk}".png
