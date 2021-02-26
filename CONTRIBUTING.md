# Contributing to OpenSkyStacker

Thanks for your interest in contributing to OpenSkyStacker! Here are a few things to keep in mind before you open a pull request.

## Coding Style

This project adheres to [Qt coding style](https://wiki.qt.io/Qt_Coding_Style), so before you open a pull request, make sure that the variable names, spacing, bracket style, etc. is consistent with Qt standards and with the rest of the code in this project.

## Testing Changes

OpenSkyStacker has a test suite that will verify that the basic features are working properly. It will build alongside the rest of the project. First, grab the image samples [from here](https://onedrive.live.com/download?cid=EA3654387692D1CD&resid=EA3654387692D1CD%216873&authkey=AP8nVyDkhYtALXE). Then you can run the test suite by running:

```
bin/oss-test -d <path-to-samples-dir>
```

If any tests fail, try to investigate the cause and fix it before submitting a pull request. If the cause can't be determined, open a [new issue](https://github.com/BenJuan26/OpenSkyStacker/issues).

If you've added new functionality, write a test case to accompany it.