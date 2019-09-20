#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
AVGL_HOME="$SCRIPT_DIR/../.."

export LD_LIBRARY_PATH="$AVGL_HOME/build/src"
export LUA_CPATH="$AVGL_HOME/build/src/?.so"
export LUA_PATH="$LUA_PATH;$AVGL_HOME/lua/?.lua"

lua5.3 $*
