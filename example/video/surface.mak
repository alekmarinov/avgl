# lib/gui/avgl/example/video/surface.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_surface
VERSION=1.0
OBJS=surface.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
