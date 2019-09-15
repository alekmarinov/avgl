add_executable(test_avgl 
    test.c
    test_avgl.c
    test_event.c
    test_oop.c
    test_sprite.c
    test_surface.c
    test_visible.c
    test_window.c
    test_widgets.c
)
target_link_libraries (test_avgl avgl)
