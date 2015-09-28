# example/audio/CMakeLists.txt

IF(WITH_DIRECTFB OR WITH_SDL)
	INCLUDE_DIRECTORIES(
			/usr/local/include/SDL
			/usr/include/SDL
			/usr/local/include/directfb
			/usr/include/directfb
	)

	ADD_EXECUTABLE(audio audio.c)
ENDIF(WITH_DIRECTFB OR WITH_SDL)

