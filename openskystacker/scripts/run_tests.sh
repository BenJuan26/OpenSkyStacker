cd "${0%/*}"

#!/bin/bash
LCOV=$(which lcov | head -1)
GENHTML=$(which genhtml | head -1)

if [ "$LCOV" == "" ] || [ "$GENHTML" == "" ]; then
    echo "ERROR: Must have lcov and genhtml installed"
    exit 1
fi

BASE_DIR=../..
BUILD_DIR=$BASE_DIR/build

UNAME="$(uname)"
if [ "$UNAME" = "Darwin" ];then
    OBJECTS_DIR=$BUILD_DIR/o/macx
else
    OBJECTS_DIR=$BUILD_DIR/o/linux
fi

BIN_DIR=$BASE_DIR/bin
TEST_BIN=$BIN_DIR/testoss

SRC_DIR=$BASE_DIR/src

set -e

# Zero the counts before we run the tests
$LCOV -d $OBJECTS_DIR -z

# Run the tests using a virtual frame buffer (since we are headless)
$XVFB $TEST_BIN

# Parse gcov results
$LCOV -d $OBJECTS_DIR -b $SRC_DIR -c -o $BUILD_DIR/coverage.info

# Filter out unwanted files
if [ "$UNAME" = "Darwin" ]; then
    $LCOV -r "$BUILD_DIR/coverage.info" "/usr/local/*" "*/src/processing/exif.*" "*/Qt*.framework/*" "*/build/moc/*" "*/build/ui/*" "*/v1/*" -o "$BUILD_DIR/coverage-filtered.info"
else
    $LCOV -r "$BUILD_DIR/coverage.info" "/usr/include/*" QtCore QtGui QtTest QtWidgets "build/moc/*" "build/ui/*" -o "$BUILD_DIR/coverage-filtered.info"
fi

# Build html from coverage results
$GENHTML -o $BUILD_DIR/html $BUILD_DIR/coverage-filtered.info

# Zero the counts again
$LCOV -d $OBJECTS_DIR -z
