# src/CMakeLists.txt

SET(${PROJECT_NAME}_SOURCES
	av_rect.c
	av_audio.c
	av_video.c
	av_input.c
	av_surface.c
	av_graphics.c
	av_player.c
	av_window.c
	av_event.c
	av_system.c
	av_timer.c
	av_sound.c
	core/av_list.c
	core/av_hash.c
	core/av_tree.c
	core/av_thread.c
	core/av_torb.c
	core/av_prefs.c
	core/av_log.c
	media/av_media.c
	media/av_scaler.c
	scripting/av_scripting.c
	sound/av_sound_wave.c
)

SET(${PROJECT_NAME}_LIBRARIES m)

IF(WITH_XINE)
	FIND_LIBRARY(XINE_LIB xine PATHS
		/usr/local/lib
		/usr/lib
	)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		player/av_player_xine.c
	)
	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} ${XINE_LIB})
ENDIF(WITH_XINE)

IF(WITH_FFMPEG)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		media/ffmpeg/av_media_ffmpeg.c
		media/swscale/av_scaler_swscale.c
		player/av_player_ffplay.c
	)
	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} avcodec avformat ${SWSCALE_LIB})
ENDIF(WITH_FFMPEG)

IF(WITH_VLC)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		player/av_player_vlc.c
	)
	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} vlc)
ENDIF(WITH_VLC)

INCLUDE(FindSDL)
IF(WITH_SDL AND SDL_FOUND)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		audio/av_audio_sdl.c
		video/av_video_sdl.c
		video/av_video_cursor_sdl.c
		video/av_video_surface_sdl.c
		video/av_video_surface_overlay_sdl.c
		video/av_video_surface_overlay_buffered_sdl.c
		input/av_input_sdl.c
		system/av_system_sdl.h
		system/av_system_sdl.c
		system/av_system_sdl_param.c
		timer/av_timer_sdl.c
	)
	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} ${SDL_LIBRARY})

	INCLUDE(FindSDL_net)
	IF (SDL_NET_FOUND)
		SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} ${SDL_NET_LIBRARIES})
	ENDIF (SDL_NET_FOUND)
ENDIF(WITH_SDL AND SDL_FOUND)

IF(WITH_DIRECTFB)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		audio/av_audio_dfb.c
		video/av_video_dfb.c
		video/av_video_surface_dfb.c
		input/av_input_dfb.c
		timer/av_timer_dfb.c
		system/av_system_dfb.h
		system/av_system_dfb.c
		system/av_system_dfb_param.c
	)
	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} directfb)
ENDIF(WITH_DIRECTFB)

IF(WITH_CAIRO)
	SET(${PROJECT_NAME}_SOURCES
		${${PROJECT_NAME}_SOURCES}
		graphics/av_graphics_cairo.c
		graphics/av_graphics_surface_cairo.c
	)

	FIND_LIBRARY(LIB_CAIRO cairo PATHS
		/usr/local/lib
		/usr/lib
	)

	FIND_PATH(CAIRO_INCLUDE_DIR cairo.h
		HINTS
			ENV CAIRODIR
		PATH_SUFFIXES include/cairo include
	)

	SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} ${LIB_CAIRO} "-L${CMAKE_INSTALL_PREFIX}/lib -lpixman-1" "-lfreetype -lz -lpng")
ENDIF(WITH_CAIRO)

INCLUDE_DIRECTORIES(
	${${PROJECT_NAME}_SOURCE_DIR}/include
	${CAIRO_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	/usr/include
	/usr/local/include
)

LINK_LIBRARIES(${${PROJECT_NAME}_LIBRARIES})
ADD_LIBRARY(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
INSTALL_TARGETS(/lib ${PROJECT_NAME})
