@echo off
rem ..\..\build\vc\Debug\lua.exe test_avgl.lua
set PATH=%PATH%;..\..\build\vc\Debug
set LUA_CPATH=..\..\build\vc\Debug\?.dll
set LUA_PATH=%LUA_PATH%;../../lua/?.lua
lua53 -lluarocks.require %1
