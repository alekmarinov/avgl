# example/window/CMakeLists.txt

IF(WITH_DIRECTFB OR WITH_SDL)
	ADD_EXECUTABLE(widgets widgets.c)
	ADD_EXECUTABLE(modals modals.c)
ENDIF(WITH_DIRECTFB OR WITH_SDL)

ADD_EXECUTABLE(windows windows.c)
