# lib/gui/avgl/extensions/xembed/av_xembed.mak

TARGET=av_xembed
VERSION=0.9.0
OBJS=av_xembed_register.o av_xembed_window.o av_xembed.o av_xembed_surface.o

EXTRA_INCS=-I. -I../.. -I../../include -I$(LRUN)/lib/gui/xembed/include
EXTRA_LIBS=-L../../src -lavgl -L$(LRUN)/lib/gui/xembed/src -lxembed
