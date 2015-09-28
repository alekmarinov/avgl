# lib/gui/avgl/example/benchmark/Makefile

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_sdlbench
VERSION=1.0
OBJS=sdlbench.o
EXTRA_INCS=$(shell pkg-config sdl --cflags) 
EXTRA_LIBS=$(shell pkg-config sdl --libs) 
include $(LRUN)/config/make/Application.mak 
