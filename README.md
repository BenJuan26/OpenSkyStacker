# OpenSkyStacker

Multi-platform astroimaging stacker built in Qt and leveraging OpenCV.

## Setup

Qt and OpenCV are required for this project. Instructions on how to install OpenCV with Qt on Linux and Windows can be found [here](https://wiki.qt.io/OpenCV_with_Qt).

## Building

The `include` and `lib` paths in [the project file](OpenSkyStacker.pro) are set according to my installation. They will likely need to be modified for each installation of OpenCV and Qt.

The project file is set up so that it should build on Windows, Linux, and macOS. It has been tested on each, but your results may vary.