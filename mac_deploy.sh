#!/bin/bash

DEFAULT_APP_PATH=build/OpenSkyStacker.app
APP_PATH=$1

if [ "$1" = "" ]; then
    APP_PATH=$DEFAULT_APP_PATH
fi

if [ ! -d $APP_PATH ]; then
    echo "App doesn't exist at $APP_PATH"
    echo "You can specify the location as an argument."
    echo "Example:"
    echo "./mac_deploy.sh build/OpenSkyStacker.app"
    exit 1
fi

BASE_LIB_PATH=$APP_PATH/Contents/Frameworks

error_exit() {
    echo $1
    exit 1
}

fix_lib() {
    local parent=$1
    local child=$2

    local child_full=$(otool -L $BASE_LIB_PATH/$parent | grep $child | awk '{print $1}')

    install_name_tool -change $child_full @executable_path/../Frameworks/$child \
        $BASE_LIB_PATH/$parent || error_exit "Couldn't set lib for $parent"
    echo "Fixed lib from $parent to $child"
}

fix_rpath() {
    local next_path_is_rpath=0
    local old_rpath
    while read -r line; do
        if [[ "$line" =~ "cmd LC_RPATH" ]]; then
            next_path_is_rpath=1
        fi

        if [ $next_path_is_rpath -eq 1 ] && [[ $line =~ path ]]; then
            old_rpath=$(echo $line | awk '{print $2}')
            next_path_is_rpath=0
        fi
    done < <(otool -l $BASE_LIB_PATH/$1)

    if [ "$old_rpath" = "" ]; then
        echo "Couldn't find rpath for $1"
        return 1
    fi

    install_name_tool -rpath $old_rpath @executable_path/../Frameworks $BASE_LIB_PATH/$1 \
        || error_exit "Couldn't set rpath for $1"
    echo "Fixed rpath for $1"
}

fix_id() {
    install_name_tool -id @executable_path/../Frameworks/$1 $BASE_LIB_PATH/$1 || error_exit "Couldn't set id for $1"
    echo "Fixed id for $1"
}


deploy_bin=$(locate macdeployqt | head -1)
if [ "$deploy_bin" = "" ]; then
    echo "Couldn't find macdeployqt"
    exit 1
fi

echo "Running macdeployqt..."
OPENCV_LIBPATH=$(pkg-config --libs-only-L opencv | awk -v ORS=" " '{
    gsub("-L","");
    for(i=1; i<=NF; i++){
        print "-libpath="$i;
    }
}')
LIBRAW_LIBPATH=$(pkg-config --libs-only-L libraw | awk -v ORS=" " '{
    gsub("-L","");
    for(i=1; i<=NF; i++){
        print "-libpath="$i;
    }
}')
$deploy_bin $APP_PATH $OPENCV_LIBPATH $LIBRAW_LIBPATH || error_exit "Error running macdeployqt"
echo "Done"
echo ""

fix_lib libIexMath-2_2.12.dylib libIex-2_2.12.dylib
fix_lib libIlmThread-2_2.12.dylib libIex-2_2.12.dylib
fix_lib libImath-2_2.12.dylib libIex-2_2.12.dylib
