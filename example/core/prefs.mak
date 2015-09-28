# lib/gui/avgl/example/core/prefs.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_prefs
VERSION=1.0
OBJS=prefs.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
