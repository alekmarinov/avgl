# include/CMakeLists.txt

INSTALL(FILES
			av.h
			av_audio.h
			av_sound.h
			av_event.h
			av_graphics.h
			av_hash.h
			av_input.h
			av_keys.h
			av_list.h
			av_log.h
			av_media.h
			av_player.h
			av_prefs.h
			av_rect.h
			av_scripting.h
			av_surface.h
			av_system.h
			av_thread.h
			av_timer.h
			av_torb.h
			av_tree.h
			av_video.h
			av_window.h
			avgl.h
			${${PROJECT_NAME}_BINARY_DIR}/av_config.h
		DESTINATION include/avgl
		PERMISSIONS OWNER_WRITE OWNER_READ
					GROUP_READ WORLD_READ)
