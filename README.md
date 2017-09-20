# OpenSkyStacker

Multi-platform deep-sky stacker for astrophotography.

[![Build Status](https://travis-ci.org/BenJuan26/OpenSkyStacker.svg?branch=master)](https://travis-ci.org/BenJuan26/OpenSkyStacker) [![Documentation](https://codedocs.xyz/BenJuan26/OpenSkyStacker.svg)](https://codedocs.xyz/BenJuan26/OpenSkyStacker/)

OpenSkyStacker assists in the processing of deep-sky images. *Stacking* in this context means taking the average of several exposures of the same object to reduce the noise and boost the signal-to-noise ratio. This is especially helpful in the field of astrophotography because many objects of interest are so dim that, without processing, they might be indistinguishable from noise.

OpenSkyStacker is not unique in what it accomplishes, as there is other stacking software out there, but it is unique in that it is free, open-source, and available for nearly any operating system.

## Download

[Download here](https://github.com/BenJuan26/OpenSkyStacker/releases) for Windows, Mac, and Ubuntu. Users on other distros can [compile from source](#build).

## Build

### Dependencies

This software depends on Qt5, OpenCV, and LibRaw. On Debian-like Linux distros, these can be installed by:

```
sudo apt-get install qt5-default libopencv-dev libraw-dev
```

On Mac, these can be installed using Homebrew:

```
brew install qt opencv libraw
```

### Building

Use `qmake` to generate the `Makefile`:

```
qmake qt=qt5
```

Then use `make` to build OpenSkyStacker:

```
make
```

This will compile the program to the `bin/` directory.

### Releasing for Mac

Qt provides the somewhat-helpful `macdeployqt` program to deploy Qt apps for Mac. However, it's not perfect: in my experience it doesn't correctly change the absolute paths of some libraries. For that reason, two scripts are provided for deploying on Mac: [mac_deploy.sh](src/scripts/mac_deploy.sh) and [create_dmg.sh](src/scripts/create_dmg.sh). The former will run `macdeployqt` and fix anything it may have missed, and the latter will create a pretty DMG image ready for release.

```
cd src
./mac_deploy.sh
./create_dmg.sh
```

## Testing

To build the test executable, run qmake with the addition of `CONFIG+=test`:

```
qmake CONFIG+=test
make
```

The tests can then be run by executing `bin/testoss` or `bin/testoss.exe`.
