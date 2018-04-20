FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y git qt5-default libraw-dev libccfits-dev wget python-pip \
    cmake pkg-config libjpeg8-dev libjasper-dev libpng12-dev libtiff5-dev && \
    pip install --upgrade pip && pip install cpp-coveralls && \
    wget https://github.com/opencv/opencv/archive/3.3.0.tar.gz -O opencv-3.3.0.tar.gz && \
    tar -zxvf opencv-3.3.0.tar.gz && cd opencv-3.3.0 && mkdir build && cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr .. && \
    make -j8 && make install && ldconfig && \
    cd ../.. && rm -rf opencv-3.3.0

RUN wget -q --no-check-certificate "https://onedrive.live.com/download?cid=EA3654387692D1CD&resid=EA3654387692D1CD%216873&authkey=AP8nVyDkhYtALXE" -O samples.tar.gz && \
    tar -zxvf samples.tar.gz && rm samples.tar.gz

COPY . OpenSkyStacker
WORKDIR OpenSkyStacker
