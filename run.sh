#!/bin/sh

docker run \
    --rm \
    -it \
    -e TZ=Asia/Seoul \
    -v common-volume:/common-volume \
    -v $(pwd):/app \
    --name IPCplusplus-dev \
    jhleeeme/cpp:0.0.1 \
    bin/bash