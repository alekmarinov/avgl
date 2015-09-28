# lib/gui/avgl/src/avgl.mak

CAIRO=1
SDL=1
ifneq ($(MINGW), 1)
    XINE=1
endif
FFMPEG=0
VLC=0

TARGET=avgl
VERSION=0.9.0
OBJS=av_rect.o av_window.o av_event.o av_timer.o av_input.o av_graphics.o av_audio.o av_sound.o av_surface.o av_video.o av_system.o av_player.o \
     core/av_hash.o core/av_list.o core/av_log.o core/av_prefs.o core/av_thread.o core/av_torb.o core/av_tree.o \
     sound/av_sound_wave.o \
     media/av_media.o media/av_scaler.o \
     scripting/av_scripting.o

EXTRA_INCS=-I.. -I../include
EXTRA_LIBS=-lpthread-2

# CAIRO
ifeq ($(CAIRO), 1)
  OBJS+=graphics/av_graphics_cairo.o graphics/av_graphics_surface_cairo.o
  EXTRA_INCS+=$(shell pkg-config cairo --cflags)
  EXTRA_LIBS+=$(shell pkg-config cairo --libs)
  EXTRA_DEFS+=-DWITH_GRAPHICS_CAIRO
endif

# SDL
ifeq ($(SDL), 1)
  OBJS+=system/av_system_sdl.o system/av_system_sdl_param.o \
    video/av_video_sdl.o video/av_video_cursor_sdl.o video/av_video_surface_overlay_buffered_sdl.o video/av_video_surface_overlay_sdl.o video/av_video_surface_sdl.o \
    input/av_input_sdl.o \
    timer/av_timer_sdl.o \
    audio/av_audio_sdl.o
  EXTRA_INCS+=$(shell pkg-config sdl --cflags)
  EXTRA_LIBS+=$(shell pkg-config sdl --libs)
  EXTRA_DEFS+=-DWITH_SYSTEM_SDL -DWITH_OVERLAY 

  ifeq ($(UNIX), 1)
    ifneq ($(MINGW), 1)
      EXTRA_INCS+=$(shell pkg-config xv --cflags)
      EXTRA_LIBS+=$(shell pkg-config xv --libs)
    endif
  endif
endif

# XINE
ifeq ($(XINE), 1)
  OBJS+=player/av_player_xine.o
  EXTRA_INCS+=$(shell pkg-config libxine --cflags)
  EXTRA_LIBS+=$(shell pkg-config libxine --libs)
  EXTRA_DEFS+=-DWITH_XINE
endif

# FFMPEG
ifeq ($(FFMPEG), 1)
  OBJS+=media/ffmpeg/av_media_ffmpeg.o \
    media/swscale/av_scaler_swscale.o \
    player/av_player_ffplay.o 
  EXTRA_INCS+=$(shell pkg-config libavformat libavcodec libavutil libswscale --cflags)
  EXTRA_LIBS+=$(shell pkg-config libavformat libavcodec libavutil libswscale --libs)
  EXTRA_DEFS+=-DWITH_FFMPEG -DUSE_INCLUDE_LIBAVFORMAT 
endif

# VLC?
ifeq ($(VLC), 1)
    OBJS+=player/av_player_vlc.o
    EXTRA_INCS+=$(shell pkg-config libvlc --cflags)
    EXTRA_LIBS+=$(shell pkg-config libvlc --libs)
endif

ifneq ($(MINGW), 1)
    EXTRA_DEFS+=-DWITH_X11
endif
