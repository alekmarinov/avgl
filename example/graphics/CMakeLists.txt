# example/graphics/CMakeLists.txt

IF(WITH_DIRECTFB OR WITH_SDL)
	IF(WITH_CAIRO)
	INCLUDE_DIRECTORIES(
		/usr/local/include/cairo
		/usr/include/cairo
		/usr/local/include/directfb
		/usr/include/directfb
		/usr/local/include/SDL
		/usr/include/SDL
	)
	ADD_EXECUTABLE(graphics graphics.c)
	ENDIF(WITH_CAIRO)
ENDIF(WITH_DIRECTFB OR WITH_SDL)
