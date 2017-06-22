# OpenSkyStacker

Multi-platform astroimaging stacker.

[![Documentation](https://codedocs.xyz/BenJuan26/OpenSkyStacker.svg)](https://codedocs.xyz/BenJuan26/OpenSkyStacker/)

## Setup

Qt5 is required for this project. In order to prevent some relatively severe memory issues, building for 64-bit is required.

OpenCV and LibRaw should also be installed. These can be easily sourced for most operating systems, but for the time being they've been included in the `3rdparty` directory so they can be built from source.

## Building

### qmake

With Qt installed, building is as simple as:

```
qmake
make
```

### Qt Creator

Optionally, Qt Creator can be used to build OpenSkyStacker. Open `OpenSkyStacker.pro`, configure your kit as prompted, and build the project.

## Testing

To build the test executable, run qmake with the addition of `CONFIG+=test`:

```
qmake CONFIG+=test
make
```

The tests can then be run by executing `build/testoss` or `build/testoss.exe`.
