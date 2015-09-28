# extensions/xembed/CMakeLists.txt

SET(PROJECT_NAME avxembed)

SET(${PROJECT_NAME}_SOURCES
	av_xembed.h          av_xembed.c
	av_xembed_surface.h  av_xembed_surface.c
	av_xembed_window.h   av_xembed_window.c
	av_xembed_register.h av_xembed_register.c
)

INCLUDE_DIRECTORIES(
	/usr/aviq/include
	${avgl_SOURCE_DIR}/include
	${avgl_SOURCE_DIR}/extensions/xembed
	/usr/local/include/ffmpeg
	/usr/include/ffmpeg
)

LINK_LIBRARIES(avgl xembed)
ADD_LIBRARY(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
INSTALL_TARGETS(/lib ${PROJECT_NAME})

INSTALL(FILES
		av_xembed_window.h
		av_xembed_app.h
		av_xembed.h
	DESTINATION
		include/avgl
	PERMISSIONS
		OWNER_WRITE
		OWNER_READ
		GROUP_READ
		WORLD_READ)
