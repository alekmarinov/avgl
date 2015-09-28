# lib/gui/avgl/example/core/hash.mak

LRUN=../../../../..

include $(LRUN)/config/make/Config.mak 
TARGET=av_hash
VERSION=1.0
OBJS=hash.o
EXTRA_INCS=-I../../include -I../..
EXTRA_LIBS=-L../../src -lavgl
include $(LRUN)/config/make/Application.mak 
