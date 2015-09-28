# lib/gui/avgl/example/video/widgets.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_widgets
VERSION=1.0
OBJS=widgets.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
