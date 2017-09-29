FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y git qt5-default libopencv-dev libraw-dev libccfits-dev

ADD . OpenSkyStacker

WORKDIR OpenSkyStacker
