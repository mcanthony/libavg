#!/bin/bash

set -e
set -x

checkConfig()
{
    if [[ x"${AVG_PATH}" == "x" ]]
    then
        echo ${AVG_PATH}
        echo Please set AVG_PATH and call 'source mac/avg_env.sh' before calling this script.
        exit -1 
    fi

    if [[ x"${AVG_MAC_ENV_SET}" == "x" ]]
    then
        echo Please call 'source mac/avg_env.sh' before calling this script.
        exit -1 
    fi

    PYTHON_VER=$(python --version 2>&1)
    if [[ ${PYTHON_VER:7:3} != "2.7" ]]
    then
        echo libavg expects python 2.7. Found version ${PYTHON_VER:7:3}
        exit -1 
    fi

    PYTHON_BITS=$(python -c 'import struct;print( 8 * struct.calcsize("P"))')
    if [[ ${PYTHON_BITS} != "64" ]]
    then
        echo libvg expects a 64-bit version of python. Found a ${PYTHON_BITS}-bit version.
        exit -1
    fi
}

clean()
{
    rm -rf ${AVG_PATH}/bin/
    rm -rf ${AVG_PATH}/lib/
    sudo rm -rf ${AVG_PATH}/include/

    mkdir ${AVG_PATH}/bin
    mkdir ${AVG_PATH}/lib
    mkdir ${AVG_PATH}/include
}

buildLib()
{
    LIBNAME=$1
    CONFIG_ARGS=$2

    echo --------------------------------------------------------------------
    cd ${LIBNAME}
    ./configure --prefix=${AVG_PATH} ${CONFIG_ARGS}
    make clean
    make -j5
    make install
    cd ..
}

buildSDL()
{
    cd SDL2-2.0.4
    CXXFLAGS="-mmacosx-version-min=10.6 -DMAC_OS_X_VERSION_MIN_REQUIRED=1050" LDFLAGS="-mmacosx-version-min=10.6" ./configure --disable-shared --disable-video-x11 --without-x --prefix=${AVG_PATH}
    make clean
    make -j5
    make install
    cd ..
}

buildglib()
{
    echo --------------------------------------------------------------------
    cd glib-2.29.2
    LDFLAGS="-framework ApplicationServices $LDFLAGS -lresolv" ./configure  --prefix=${AVG_PATH} --disable-shared --enable-static 
    make clean
    make -j5
    make install
    cd ..
}

buildfontconfig()
{
    echo --------------------------------------------------------------------
    cd fontconfig-2.11.1
    LDFLAGS="-framework ApplicationServices ${LDFLAGS}" ./configure --prefix=${AVG_PATH} --disable-shared --with-add-fonts=/Library/Fonts,/System/Library/Fonts,~/Library/Fonts --with-configdir=/etc/fonts --with-cache-dir=${HOME}/.fontconfig --disable-docs
    make clean
    make -j5
    sudo make install
    # Workaround for fontconfig bug: Install fonts.conf in the correct place.
    sudo cp ../../etc/fonts/fonts.conf /etc/fonts/fonts.conf
    sudo chown -R `whoami` ~/.fontconfig
    cd ..    
}

buildgdkpixbuf()
{
    echo --------------------------------------------------------------------
    cd gdk-pixbuf-2.23.3
    LDFLAGS="-framework ApplicationServices $LDFLAGS -lresolv" ./configure --prefix=${AVG_PATH} --disable-shared --with-included-loaders
    make clean
    make -j5
    make install
    cd ..
}

buildlibrsvg()
{
    echo --------------------------------------------------------------------
    cd librsvg-2.34.0
    autoreconf --force --install
    LDFLAGS=`xml2-config --libs` CPPFLAGS=`xml2-config --cflags` ./configure --prefix=${AVG_PATH} --disable-shared --disable-gtk-theme --disable-tools
    make clean
    make -j5
    make install
    cd ..
}

buildboost()
{
    echo --------------------------------------------------------------------
    cd boost_1_54_0
    ./bootstrap.sh --prefix=${AVG_PATH} --with-libraries=python,thread,date_time,system
    ./bjam clean
    ./bjam install
    cd ..
    rm -f ../lib/libboost_thread.dylib
    rm -f ../lib/libboost_python.dylib
    rm -f ../lib/libboost_date_time.dylib
    rm -f ../lib/libboost_system.dylib
}

checkConfig

clean

cd ../deps

buildLib libtool-2.4.6
buildLib autoconf-2.69
buildLib automake-1.15
buildLib nasm-2.10.09
buildLib libjpeg-turbo-1.3.0 "--host x86_64-apple-darwin --disable-shared NASM=${AVG_PATH}/bin/nasm"
buildLib tiff-3.8.2 --disable-shared 
buildLib libpng-1.2.41 --disable-shared
buildLib pkg-config-0.20
buildLib yasm-1.2.0 
buildLib libav-9.9 "--arch=x86_64 --disable-debug --enable-pthreads --enable-runtime-cpudetect"

buildSDL
buildLib gettext-0.18.1.1 "--disable-shared --with-included-gettext --disable-csharp  --disable-libasprintf"
buildglib

buildLib freetype-2.5.0.1 "--disable-shared --with-old-mac-fonts"
buildLib expat-2.0.0 --disable-shared

buildfontconfig

buildLib pixman-0.22.0 --disable-shared
buildLib cairo-1.10.2 "--disable-shared --enable-xlib=no --enable-xlib-xrender=no --enable-quartz=no --enable-quartz-font=no --enable-quartz-image=no --enable-ps=no --enable-pdf=no --enable-svg=no"
buildLib pango-1.24.4 "--disable-shared --without-x --with-included-modules=yes"

buildgdkpixbuf
buildlibrsvg

buildboost

buildLib libdc1394-2.2.1 "--disable-shared --disable-doxygen-doc --without-x"

cd ../libavg
