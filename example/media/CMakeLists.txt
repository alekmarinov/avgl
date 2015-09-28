# example/media/CMakeLists.txt

IF(WITH_DIRECTFB OR WITH_SDL)
	IF(WITH_FFMPEG)
	INCLUDE_DIRECTORIES(
			/usr/local/include/SDL
			/usr/include/SDL
			/usr/local/include/directfb
			/usr/include/directfb
			/usr/include/ffmpeg
			/usr/local/include/ffmpeg
	)

	ADD_EXECUTABLE(media media.c)
	ENDIF(WITH_FFMPEG)
ENDIF(WITH_DIRECTFB OR WITH_SDL)
