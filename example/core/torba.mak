# lib/gui/avgl/example/core/torba.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_torba
VERSION=1.0
OBJS=torba.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
