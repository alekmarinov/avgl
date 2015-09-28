# lib/gui/avgl/example/core/threads.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_threads
VERSION=1.0
OBJS=threads.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
