# example/benchmark/CMakeLists.txt

IF(WITH_SDL)
	FIND_PATH(SDL_HEADER
			  SDL.h
			  PATHS /usr/local/include/SDL 
					/usr/include/SDL
	)
	IF(SDL_HEADER)
		INCLUDE_DIRECTORIES(${SDL_HEADER})

		LINK_LIBRARIES(SDL)
		ADD_EXECUTABLE(sdlbench sdlbench.c)
	ENDIF(SDL_HEADER)

ENDIF(WITH_SDL)
