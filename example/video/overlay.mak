# lib/gui/avgl/example/video/overlay.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_overlay
VERSION=1.0
OBJS=overlay.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
