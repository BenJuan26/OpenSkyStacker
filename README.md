# OpenSkyStacker

Multi-platform astroimaging stacker.

## Setup

Qt 5 is required for this project. In order to prevent some relatively severe memory issues, building for 64-bit is required.

## Building

The project is set up so that it should build on Windows, Linux, and macOS. It has been tested on each, but your results may vary.

Third-party library binaries have been included for simplicity's sake. They are located in the [3rdparty directory](3rdparty) along with their respective licenses. Should the proper binary for your workstation not be included, the libraries can be compiled from their source.

### Qt Creator

Qt Creator is the most user-friendly method for building. Once the proper kit for your machine is configured, the initial build can be done with these steps:

* `Build -> Clean Project "OpenSkyStacker"`
* `Build -> Run qmake`
* `Build -> Rebuild project "OpenSkyStacker"`

After the initial build, simply running `Build -> Build Project "OpenSkyStacker"` or clicking the hammer in the bottom-left corner of Qt Creator should be sufficient. The only exception to this is if the `.pro` file is changed. In this case, run `Build -> Run qmake` before building.

### qmake

Although Qt Creator is recommended, the project can be built with `qmake` alone. Detailed instructions can be found [here](http://doc.qt.io/qt-5/qmake-running.html), but generally you can run `qmake OpenSkyStacker.pro` from the project directory, and `make` from the generated build directory (e.g. `build-OpenSkyStacker-Desktop_Qt_5_8_0_clang_64bit-Debug`).