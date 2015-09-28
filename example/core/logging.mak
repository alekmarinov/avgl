# lib/gui/avgl/example/core/logging.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_logging
VERSION=1.0
OBJS=logging.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
