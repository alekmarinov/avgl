# lib/gui/avgl/example/core/mutex.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_mutex
VERSION=1.0
OBJS=mutex.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
