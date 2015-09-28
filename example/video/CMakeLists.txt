# example/video/CMakeLists.txt

IF(WITH_DIRECTFB OR WITH_SDL)
	INCLUDE_DIRECTORIES(
			/usr/local/include/SDL
			/usr/include/SDL
			/usr/local/include/directfb
			/usr/include/directfb
	)
	ADD_EXECUTABLE(video video.c)
	ADD_EXECUTABLE(surface surface.c)
	ADD_EXECUTABLE(overlay overlay.c)
ENDIF(WITH_DIRECTFB OR WITH_SDL)
