# lib/gui/avgl/example/video/windows.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_windows
VERSION=1.0
OBJS=windows.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
