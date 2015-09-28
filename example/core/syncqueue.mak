# lib/gui/avgl/example/core/syncqueue.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_syncqueue
VERSION=1.0
OBJS=syncqueue.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
