FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y git qt5-default libopencv-dev libraw-dev libccfits-dev wget

ADD . OpenSkyStacker

WORKDIR OpenSkyStacker

RUN wget --no-check-certificate "https://onedrive.live.com/download?cid=EA3654387692D1CD&resid=EA3654387692D1CD%216873&authkey=AP8nVyDkhYtALXE" -O samples.tar.gz && \
    tar -zxvf samples.tar.gz -C src/images