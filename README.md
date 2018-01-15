# OpenSkyStacker

Multi-platform deep-sky stacker for astrophotography.

[![Build Status](https://travis-ci.org/BenJuan26/OpenSkyStacker.svg?branch=master)](https://travis-ci.org/BenJuan26/OpenSkyStacker) [![Documentation](https://codedocs.xyz/BenJuan26/OpenSkyStacker.svg)](https://codedocs.xyz/BenJuan26/OpenSkyStacker/) [![Coverage Status](https://coveralls.io/repos/github/BenJuan26/OpenSkyStacker/badge.svg)](https://coveralls.io/github/BenJuan26/OpenSkyStacker)

OpenSkyStacker assists in the processing of deep-sky images. *Stacking* in this context means taking the average of several exposures of the same object to reduce the noise and boost the signal-to-noise ratio. This is especially helpful in the field of astrophotography because many objects of interest are so dim that, without processing, they might be indistinguishable from noise.

OpenSkyStacker is not unique in what it accomplishes, as there is other stacking software out there, but it is unique in that it is free, open-source, and available for nearly any operating system.

## Download

[Download here](https://github.com/BenJuan26/OpenSkyStacker/releases) for Windows, Mac, and Ubuntu. Linux users on other distros can [compile from source](https://github.com/BenJuan26/OpenSkyStacker/wiki/Build-from-source).

## Getting started

To start stacking, load your images using the buttons to the top-right. For an explanation of light frames, dark frames, etc., I would recommend [DeepSkyStacker's page on the matter](http://deepskystacker.free.fr/english/theory.htm).

When you load light frames, the first one is set as the reference image by default and displayed in bold. You can choose a different reference image by selecting it from the list, right-clicking, and selecting `Set As Reference` from the menu. Images can be checked or unchecked to include or exclude them from stacking.

To stack the checked images, click the `Align and Stack` button. You'll be prompted where to save the resulting image, which is always a 32-bit TIFF file, and the stacking process will begin.

### Troubleshooting

If you're met with an error or if the resulting image looks skewed or undesirable, try changing the star detection threshold using the `Options` button. You can see how many stars are detected at the current threshold by clicking the `Detect Stars` button. You should aim for about 60-80 detected stars for the best results.

**Note**: OpenSkyStacker uses stars for alignment, so it is unsuitable for planetary, lunar, or solar stacking.

## Build from source

See the [wiki page](https://github.com/BenJuan26/OpenSkyStacker/wiki/Build-from-source) for instructions to build OpenSkyStacker from the source code.
