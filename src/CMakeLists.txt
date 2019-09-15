
find_package(Threads)
find_package(Cairo REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2Image REQUIRED)

if (CAIRO_FOUND)
    set(cairo_sources
        cairo/av_graphics_cairo.c
        cairo/av_graphics_surface_cairo.c
    )
    ADD_DEFINITIONS(-DWITH_GRAPHICS_CAIRO)
endif()

if (SDL2_FOUND)
    set(sdl_sources
        sdl/av_audio_sdl.c
        sdl/av_bitmap_sdl.c
        sdl/av_core_sdl.c
        sdl/av_display_cursor_sdl.c
        sdl/av_display_sdl.c
        sdl/av_input_sdl.c
        sdl/av_surface_sdl.c
        sdl/av_system_sdl.c
        sdl/av_timer_sdl.c
    )
    ADD_DEFINITIONS(-DWITH_SYSTEM_SDL)
endif()

set(core_sources
    core/av_hash.c
    core/av_list.c
    core/av_log.c
    core/av_oop.c
    # core/av_prefs.c
    core/av_stdc.c
    core/av_thread.c
    core/av_tree.c
)

set(sources 
    # av_audio.c
    av_bitmap.c
    av_display.c
    av_event.c
    avgl.c
    av_graphics.c
    av_input.c
    # av_media.c
    # av_player.c
    av_rect.c
    # av_scaler.c
    # av_scripting.c
    # av_sound.c
    # av_sound_wave.c
    av_sprite.c
    av_surface.c
    av_system.c
    av_timer.c
    av_visible.c
    av_window.c
    ${core_sources}
    ${cairo_sources}
    ${sdl_sources}
)

add_library(avgl SHARED ${sources})
target_include_directories(avgl
    PUBLIC  ${AVGL_INCLUDE_DIR}
    PRIVATE ${AVGL_SOURCE_DIR}
    PRIVATE ${CAIRO_INCLUDE_DIRS}
    PRIVATE ${SDL2_INCLUDE_DIRS}
)

target_link_libraries (avgl
    PUBLIC m
    PUBLIC ${CMAKE_THREAD_LIBS_INIT}
    PUBLIC ${CAIRO_LIBRARIES}    
    PUBLIC ${SDL2_LIBRARIES}
    PUBLIC ${SDL2IMAGE_LIBRARIES}
)
