# example/extensions/xembed/CMakeLists.txt

LINK_LIBRARIES(avgl avxembed)

ADD_EXECUTABLE(xembed_test xembed_test.c)
ADD_EXECUTABLE(xembed_window xembed_window.c)
ADD_EXECUTABLE(xembed_app_test xembed_app_test.c)
ADD_EXECUTABLE(grid grid.c)
