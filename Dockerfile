FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y git qt5-default libopencv-dev libraw-dev libccfits-dev wget python-pip && \
    pip install --upgrade pip && pip install cpp-coveralls

ADD . OpenSkyStacker

WORKDIR OpenSkyStacker

RUN wget -q --no-check-certificate "https://onedrive.live.com/download?cid=EA3654387692D1CD&resid=EA3654387692D1CD%216873&authkey=AP8nVyDkhYtALXE" -O samples.tar.gz && \
    tar -zxvf samples.tar.gz
