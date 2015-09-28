# lib/gui/avgl/example/core/list.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_list
VERSION=1.0
OBJS=list.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
