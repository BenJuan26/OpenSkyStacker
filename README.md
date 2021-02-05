# OpenSkyStacker

Multi-platform stacker for deep-sky astrophotography.

[![Build Status](https://travis-ci.org/BenJuan26/OpenSkyStacker.svg?branch=master)](https://travis-ci.org/BenJuan26/OpenSkyStacker) [![Documentation](https://codedocs.xyz/BenJuan26/OpenSkyStacker.svg)](https://codedocs.xyz/BenJuan26/OpenSkyStacker/) [![Coverage Status](https://coveralls.io/repos/github/BenJuan26/OpenSkyStacker/badge.svg)](https://coveralls.io/github/BenJuan26/OpenSkyStacker)

OpenSkyStacker assists in the processing of deep-sky images. *Stacking* in this context means taking the average of several exposures of the same object to reduce the noise and boost the signal-to-noise ratio. This is especially helpful in the field of astrophotography because many objects of interest are so dim that, without processing, they might be indistinguishable from noise.

## Download

[Download here](https://github.com/BenJuan26/OpenSkyStacker/releases) for Windows and Mac. Linux users can [compile from source](https://github.com/BenJuan26/OpenSkyStacker/wiki/Build-from-source).

## Getting started

To start stacking, load your images using the buttons to the top-right. For an explanation of light frames, dark frames, etc., I would recommend [DeepSkyStacker's page on the matter](http://deepskystacker.free.fr/english/theory.htm).

When you load light frames, the first one is set as the reference image by default and displayed in bold. You can choose a different reference image by selecting it from the list, right-clicking, and selecting `Set As Reference` from the menu. Images can be checked or unchecked to include or exclude them from stacking.

To stack the checked images, click the `Align and Stack` button. You'll be prompted where to save the resulting image, which is always a 32-bit TIFF file, and the stacking process will begin.

### Troubleshooting

If you're met with an error or if the resulting image looks skewed or undesirable, try changing the star detection threshold using the `Options` button. You can see how many stars are detected at the current threshold by clicking the `Detect Stars` button. You should aim for about 60-80 detected stars for the best results.

**Note**: OpenSkyStacker uses stars for alignment, so it is unsuitable for planetary, lunar, or solar stacking.

## Command Line

The command line program is `openskystacker-cl`.

```
Usage: openskystacker-cl [options]
Multi-platform deep-sky stacker for astrophotography.

Options:
  -v, --version   Displays version information.
  -h, --help      Displays this help.
  -f <list>       Image list JSON file.
  -s              Detect and print the number of stars in the reference image
                  with the given threshold, then exit. Ignores all other options
                  except -f and -t.
  -o <output>     Output image file.
  -t <threshold>  Star detection threshold (1-100). Default: 20
  -j <threads>    Number of processing threads. Default: 1
```

On Mac, the command-line binary is located inside the application package at `OpenSkyStacker.app/Contents/bin/openskystacker-cl`. To make it more portable to run, you might want to put a symbolic link into a system directory. Example:

```bash
ln -s OpenSkyStacker.app/Contents/bin/openskystacker-cl /usr/local/bin/openskystacker-cl
```

It can then be run anywhere with `openskystacker-cl`.

## Build from source

See the [wiki page](https://github.com/BenJuan26/OpenSkyStacker/wiki/Build-from-source) for instructions to build OpenSkyStacker from the source code.
