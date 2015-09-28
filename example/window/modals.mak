# lib/gui/avgl/example/video/modals.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_modals
VERSION=1.0
OBJS=modals.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
