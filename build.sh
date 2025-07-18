#!/usr/bin/env sh

export CFLAGS="-I$EMSCRIPTEN/system/local/include  -sUSE_ZLIB=1"

export CXXFLAGS="-I$EMSCRIPTEN/system/local/include -sUSE_ZLIB=1"

export LDFLAGS="-L$EMSCRIPTEN/system/local/lib -sERROR_ON_UNDEFINED_SYMBOLS=0 -sUSE_ZLIB=1"

export CONFIGURE_ARGS="--host=wasm32-unknown-emscripten
--with-cxx=17
--enable-utf8
--enable-universal
--enable-dynamicloader=no
--disable-shared
--disable-exceptions
--disable-richtext
--disable-aui
--disable-xlocale
--disable-ribbon
--without-libtiff
--without-expat
--with-libpng=builtin
"

$EMSCRIPTEN/emconfigure ./configure $CONFIGURE_ARGS

$EMSCRIPTEN/emmake make
