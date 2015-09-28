# lib/gui/avgl/example/video/video.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_video
VERSION=1.0
OBJS=video.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
