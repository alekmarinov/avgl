/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_system.c                                        */
/* Description:   Abstract system class                              */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <av_system.h>
#include <av_log.h>
#include <av_prefs.h>

/* Default preferences in case not defined in the configuration file */
static const char* _default_graphics_driver = "cairo";
static const char* _default_video_mode = "windowed";
static const int _default_video_xres = 1280;
static const int _default_video_yres = 1024;

static av_log_p _log = AV_NULL;

typedef struct window_rect_pair
{
	av_window_p window;
	av_rect_t rect;
} window_rect_pair_t, *window_rect_pair_p;

typedef struct hover_info
{
	av_window_p window;
	unsigned long hover_time;
	int mouse_x;
	int mouse_y;
	av_bool_t hovered;
} hover_info_t, *hover_info_p;

static av_result_t av_system_get_audio(av_system_p self, av_audio_p* ppaudio)
{
	AV_UNUSED(self);
	AV_UNUSED(ppaudio);
	return AV_ESUPPORTED;
}

static av_result_t av_system_get_video(av_system_p self, av_video_p* ppvideo)
{
	AV_UNUSED(self);
	AV_UNUSED(ppvideo);
	return AV_ESUPPORTED;
}

static av_result_t av_system_get_input(av_system_p self, av_input_p* ppinput)
{
	AV_UNUSED(self);
	AV_UNUSED(ppinput);
	return AV_ESUPPORTED;
}

static av_result_t av_system_get_timer(av_system_p self, av_timer_p* pptimer)
{
	AV_UNUSED(self);
	AV_UNUSED(pptimer);
	return AV_ESUPPORTED;
}

static av_result_t av_system_get_graphics(av_system_p self, av_graphics_p* ppgraphics)
{
	av_result_t rc;
	if (!(self->graphics))
	{
		av_prefs_p prefs;
		const char* graphics_driver;
		char graphics_class[MAX_NAME_SIZE];

		if (AV_OK != (rc = av_torb_service_addref("prefs", (av_service_p*)&prefs)))
			return rc;
		prefs->get_string(prefs, "graphics.driver",
						  _default_graphics_driver, &graphics_driver);
		av_torb_service_release("prefs");

		strcpy(graphics_class, "graphics_");
		strcat(graphics_class, graphics_driver);

		/* create graphics */
		if (AV_OK != (rc = av_torb_create_object(graphics_class,
												(av_object_p*)&(self->graphics))))
			return rc;
	}

	*ppgraphics = self->graphics;
	return AV_OK;
}

static av_bool_t on_paint_window_root(av_window_p self, av_graphics_p graphics)
{
/* FIXME: Updating the root window is usually necessary */
	av_rect_t rect;
	self->get_rect(self, &rect);
	graphics->rectangle(graphics, &rect);
	graphics->set_color_rgba(graphics, 0.5, 0.5, 0.5, 1);
	graphics->fill(graphics, 0);
	AV_UNUSED(self);
	AV_UNUSED(graphics);
	return AV_TRUE;
}

static av_result_t av_system_create_window(av_system_p self,
										   av_window_p parent,
										   av_rect_p winrect,
										   av_window_p* ppwindow)
{
	av_result_t rc;
	av_window_p window;
	av_rect_t rect;

	if (!parent)
		parent = self->root;

	if (winrect)
		av_rect_copy(&rect, winrect);
	else
		av_rect_init(&rect, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT);

	if (AV_OK != (rc = av_torb_create_object("window", (av_object_p*)&window)))
		return rc;

	if (!(self->root))
	{
		av_prefs_p prefs;
		const char* video_mode;
		av_video_p video;
		av_video_config_t video_config;
		int xres, yres;

		if (AV_OK != (rc = av_torb_service_addref("prefs", (av_service_p*)&prefs)))
			return rc;
		prefs->get_string(prefs, "system.video.mode", _default_video_mode, &video_mode);
		prefs->get_int(prefs, "system.video.xres", _default_video_xres, &xres);
		prefs->get_int(prefs, "system.video.yres", _default_video_yres, &yres);
		av_torb_service_release("prefs");

		if (AV_OK != (rc = self->get_video(self, &video)))
			return rc;

		video_config.flags  = AV_VIDEO_CONFIG_MODE | AV_VIDEO_CONFIG_SIZE;
		if (0 == strcasecmp("FULLSCREEN", video_mode))
			video_config.mode   = AV_VIDEO_MODE_FULLSCREEN;
		else
			video_config.mode   = AV_VIDEO_MODE_WINDOWED;

		rect.x = rect.y = 0;
		if (rect.w == AV_DEFAULT || rect.h == AV_DEFAULT)
		{
			rect.w = xres;
			rect.h = yres;
		}
		video_config.width  = rect.w;
		video_config.height = rect.h;

		/* setup video mode */
		_log->info(_log, "Set video mode: %dx%d %s", video_config.width, video_config.height, video_mode);
		if (AV_OK != (rc = video->set_configuration(video, &video_config)))
			return rc;

		window->on_paint = on_paint_window_root;
		self->root = window;
	}

	window->system = self;
	window->set_parent(window, parent);
	window->set_rect(window, &rect);
	if (ppwindow)
		*ppwindow = window;
		
	window->update(window, AV_UPDATE_INVALIDATE);
	return AV_OK;
}

static void av_system_removing_window(av_system_p self, av_window_p window)
{
	/* FIXME: Consider send DESTROY event to the removing window */
	if (self->root == window)
		self->root = AV_NULL;
	if (self->focus == window)
		self->focus = AV_NULL;
	if (self->capture == window)
		self->capture = AV_NULL;


	for (self->hover_windows->first(self->hover_windows);
		 self->hover_windows->has_more(self->hover_windows);
		 self->hover_windows->next(self->hover_windows))
	{
		hover_info_p hover_info = (hover_info_p)self->hover_windows->get(self->hover_windows);
		if (hover_info->window == window)
		{
			self->hover_windows->remove(self->hover_windows);
		}
	}

	if (self->modal == window)
		self->modal = AV_NULL;
}

static void av_system_invalidate_rect(av_system_p self, av_rect_p rect)
{
	av_rect_p prect;
	if (rect->w && rect->h)
	{
		av_rect_t crect;
		self->root->get_rect(self->root, &crect);
		/*av_rect_copy(&crect, rect);*/
		if (av_rect_intersect(rect, &crect, &crect))
		{
			for (self->invalid_rects->first(self->invalid_rects);
				self->invalid_rects->has_more(self->invalid_rects);
				self->invalid_rects->next(self->invalid_rects))
			{
				av_rect_t dummy_rect;
				prect = (av_rect_p)self->invalid_rects->get(self->invalid_rects);
				if (av_rect_intersect(&crect, prect, &dummy_rect))
				{
					av_rect_extend(prect, &crect);
					return ;
				}
			}
			prect = (av_rect_p)malloc(sizeof(av_rect_t));
			av_rect_copy(prect, rect);
			self->invalid_rects->push_last(self->invalid_rects, prect);
		}
	}
}

static void av_system_invalidate_all_recurse(av_window_p window)
{
	av_list_p children;

	window->update(window, AV_UPDATE_REPAINT);
	children = window->get_children(window);

	/* update children */
	for (children->first(children); children->has_more(children); children->next(children))
	{
		av_window_p child = (av_window_p)children->get(children);
		av_system_invalidate_all_recurse(child);
	}
}

static void av_system_invalidate_all(av_system_p self)
{
	av_system_invalidate_all_recurse(self->root);
}

/*
* Clip rect with all parents storing the result to cliprect
* Returns AV_TRUE if the output cliprect is valid rectangle, AV_FALSE otherwise
* Rectangles must be given according to a same origine (e.g. in absolute coordinates)
*/
static av_bool_t av_system_clip_with_parents(av_window_p parent, av_rect_p rect, av_rect_p cliprect)
{
	av_assert(parent && rect && cliprect, "Invalid arguments passed to av_system_clip_with_parents");
	av_rect_copy(cliprect, rect);
	while (parent)
	{
		if (parent->are_children_clipped(parent))
		{
			av_rect_t prect;
			parent->get_rect(parent, &prect);
			prect.x = prect.y = 0;
			parent->rect_absolute(parent, &prect);

			if (!av_rect_intersect(&prect, cliprect, cliprect))
				return AV_FALSE; /* No intersection, the result cliprect is invalid */
		}
		parent = parent->get_parent(parent);
	}
	return AV_TRUE;
}

/* FIXME: Add the window to modality stack.
Currently only one modal window is supported at a time */
static int av_system_modal_enter(av_system_p self, av_window_p window,
								 av_modality_mode_t modality_mode)
{
	if (self->modal)
	{
		self->modal_exit(self, window, 0);
	}
	av_assert(self->modal == NULL, "Multiple modal windows are not supported yet");
	window->raise_top(window);
	window->set_visible(window, AV_TRUE);
	self->modal = window;
	self->modal_result_code = 0;

	if (AV_MODALITY_BLOCK == modality_mode)
	{
		while (self->modal && self->modal->is_visible(self->modal))
		{
			self->step(self);
		}
		self->modal = NULL;
	}
	return self->modal_result_code;
}

static void av_system_modal_exit(av_system_p self, av_window_p window, int result_code)
{
	if (self->modal == window)
	{
		av_assert(self->modal == window, "modal window is expected by modal_exit");
		window->set_visible(window, AV_FALSE);
		self->modal_result_code = result_code;
		self->modal = NULL;
	}
}

/* Recursive updating all windows intersecting given rect */
static void av_system_update_recurse(av_window_p window, av_rect_p rect, av_video_surface_p video_surface, int origin_x, int origin_y)
{
	av_rect_t winabsrect;
	av_rect_t irect;
	av_list_p children = window->get_children(window);
	av_window_p parent = window->get_parent(window);
	av_bool_t intersected;

	/* avoid updating invisible windows */
	if (AV_FALSE == window->is_visible(window))
		return;

	/*
	* the passed rect is assumed to be in absolute coordinates
	* converting the window rect in absolute coordinates
	* to make intersection under same origin
	*/
	window->get_rect(window, &winabsrect);
	winabsrect.x = winabsrect.y = 0;
	window->rect_absolute(window, &winabsrect);

	if (parent)
	{
		av_bool_t rc = av_system_clip_with_parents(parent, rect, &irect);
		av_assert(rc == AV_TRUE, "parent window clips children but no intersection here?!")
	}
	else
		av_rect_copy(&irect, rect);

	/* is there intersection with the target rect? */
	intersected = av_rect_intersect(&winabsrect, &irect, &irect);
	if (intersected || (AV_FALSE == window->are_children_clipped(window)))
	{
		if (!intersected)
			av_rect_copy(&irect, rect);

		window->on_update(window, video_surface, &irect);

		/* update children */
		for (children->first(children); children->has_more(children); children->next(children))
		{
			av_window_p child = (av_window_p)children->get(children);
			av_system_update_recurse(child, rect, video_surface, origin_x + child->origin_x, origin_y + child->origin_y);
		}
	}
}

static av_result_t av_system_update(av_system_p self)
{
	if (self->invalid_rects->size(self->invalid_rects) > 0)
	{
		/* prepare graphics */
		av_result_t rc;
		av_video_p video;
		av_video_surface_p backbuffer;

		if (AV_OK != (rc = self->get_video(self, &video)))
		{
			_log->error(_log, "Unable to obtain video object, rc = %d", rc);
			return rc; /* Return exit status */
		}

		if (AV_OK != (rc = video->get_backbuffer(video, &backbuffer)))
		{
			_log->error(_log, "Unable to obtain backbuffer surface, rc = %d", rc);
			return rc; /* Return exit status */
		}

		for (self->invalid_rects->first(self->invalid_rects);
			self->invalid_rects->has_more(self->invalid_rects);
			self->invalid_rects->next(self->invalid_rects))
		{
			av_rect_p invalid_rect = (av_rect_p)self->invalid_rects->get(self->invalid_rects);

			av_system_update_recurse(self->root, invalid_rect, backbuffer, self->root->origin_x, self->root->origin_y);
			video->update(video, invalid_rect);
		}

		/* remove and free the invalid rects */
		self->invalid_rects->remove_all(self->invalid_rects, free);
	}
	return AV_OK;
}

static void av_system_set_base_resolution(av_system_p self, int xbase, int ybase)
{
	self->xbase = xbase;
	self->ybase = ybase;
}

static void av_system_get_base_resolution(av_system_p self, int* pxbase, int* pybase)
{
	*pxbase = self->xbase;
	*pybase = self->ybase;
}

static void av_system_set_capture(av_system_p self, av_window_p window)
{
	av_assert(self->video, "video is not initialized");
	self->video->set_capture(self->video, window != AV_NULL);
	self->capture = window;
}

static av_bool_t av_system_bubble_event(av_system_p self, av_window_p window,
										av_event_p event)
{
	AV_UNUSED(self);
	/* bubbling event */
	while (window)
	{
		/* if event catch is final, then stop bubbling */
		if ((window->is_handle_events(window) && window->on_event && window->on_event(window, event)) ||
		   (!window->is_bubble_events(window))) return AV_TRUE;
		window = window->get_parent(window);
	}
	return AV_FALSE;
}

static void av_system_set_focus(av_system_p self, av_window_p window)
{
	if (self->focus != window)
	{
		av_event_t event;
		event.type = AV_EVENT_FOCUS;
		event.flags = AV_EVENT_FLAGS_WINDOW | AV_EVENT_FLAGS_DATA;

		/* trigger unfocus event */
		if (self->focus)
		{
			event.window = self->focus;
			event.data = (void*)0;
			av_system_bubble_event(self, self->focus, &event);
		}

		self->focus = window;

		/* trigger focus event */
		if (self->focus)
		{
			event.window = self->focus;
			event.data = (void*)1;
			av_system_bubble_event(self, self->focus, &event);
		}
	}
}

static av_bool_t av_system_is_focus(av_system_p self, av_window_p window)
{
	return self->focus == window;
}

static av_window_p av_system_find_window_xy(av_window_p window, int x, int y)
{
	av_window_p result = AV_NULL;
	if (window->is_visible(window))
	{
		av_list_p children = window->get_children(window);
		if (window->point_inside(window, x, y))
			result = window;

		for (children->first(children); children->has_more(children); children->next(children))
		{
			av_window_p child = (av_window_p)children->get(children);
			if ((child = av_system_find_window_xy(child, x, y)))
			{
				result = child;
 			}
		}
	}
	return result;
}

static av_bool_t av_system_add_hover_info(av_system_p self, av_window_p window, int x, int y)
{
	hover_info_p hover_info;
	av_timer_p timer;
	if (AV_OK != self->get_timer(self, &timer))
		return AV_FALSE;

	if (!window->is_handle_events(window))
		return AV_FALSE;

	for (self->hover_windows->first(self->hover_windows);
		self->hover_windows->has_more(self->hover_windows);
		self->hover_windows->next(self->hover_windows))
	{
		hover_info = (hover_info_p)self->hover_windows->get(self->hover_windows);
		if (hover_info->window == window)
		{
			/* already added, do nothing */
			return AV_FALSE;
		}
	}

	hover_info = (hover_info_p)malloc(sizeof(hover_info_t));
	hover_info->window = window;
	hover_info->hover_time = timer->now();
	hover_info->mouse_x = x;
	hover_info->mouse_y = y;
	hover_info->hovered = AV_FALSE;
	self->hover_windows->push_last(self->hover_windows, hover_info);
	return AV_TRUE;
}

static av_bool_t av_system_step(av_system_p self)
{
	av_event_t event;
	av_input_p input;
	av_timer_p timer;
	unsigned long now;

	if (AV_OK != self->get_input(self, &input))
		return AV_FALSE;

	if (AV_OK != self->get_timer(self, &timer))
		return AV_FALSE;

	now = timer->now();
	for (self->hover_windows->first(self->hover_windows);
		 self->hover_windows->has_more(self->hover_windows);
		 self->hover_windows->next(self->hover_windows))
	{
		hover_info_p hover_info = (hover_info_p)self->hover_windows->get(self->hover_windows);
		if (!hover_info->hovered && (now - hover_info->hover_time > hover_info->window->hover_delay))
		{
			event.type = AV_EVENT_MOUSE_HOVER;
			if (hover_info->window->is_visible(hover_info->window))
			{
				av_system_bubble_event(self, hover_info->window, &event);
			}
			hover_info->hovered = AV_TRUE;
		}
	}

	if (!(input->poll_event(input, &event)))
	{
		/* minimal pause to allow other threads to do something */
		timer->sleep_ms(10);
	}
	else
	{
		av_window_p target = AV_NULL;
		switch (event.type)
		{
			case AV_EVENT_MOUSE_BUTTON:
				av_assert(AV_MASK_ENABLED(event.flags, AV_EVENT_FLAGS_MOUSE_BUTTON), "invalid mouse event");
			case AV_EVENT_MOUSE_MOTION:
				av_assert(AV_MASK_ENABLED(event.flags, AV_EVENT_FLAGS_MOUSE_XY), "invalid mouse event");
			{
				if (self->capture)
					target = self->capture;
				else
				if (self->root)
				{
					av_event_type_t event_type = event.type;
					av_window_p hovered = AV_NULL;
					target = av_system_find_window_xy(self->root, event.mouse_x, event.mouse_y);
					/*if (event.type == AV_EVENT_MOUSE_MOTION)*/
					{
						if (target && self->video)
						{
							if (self->video->is_cursor_visible(self->video))
							{
								if (!target->cursor_visible)
									self->video->set_cursor_visible(self->video, AV_FALSE);
							}
							else
							{
								if (target->cursor_visible)
								{
									self->video->set_cursor_visible(self->video, AV_TRUE);
									self->video->set_mouse_position(self->video, event.mouse_x, event.mouse_y);
								}
							}
							self->video->set_cursor_shape(self->video, target->cursor);
						}
						for (self->hover_windows->first(self->hover_windows);
							self->hover_windows->has_more(self->hover_windows);
							self->hover_windows->next(self->hover_windows))
						{
							hover_info_p hover_info = (hover_info_p)self->hover_windows->get(self->hover_windows);
							if (hover_info->window == target)
								hovered = target;
							else
							if (!hover_info->window->point_inside(hover_info->window, event.mouse_x, event.mouse_y))
							{
								event.type = AV_EVENT_MOUSE_LEAVE;
								av_system_bubble_event(self, hover_info->window, &event);
								free(hover_info);
								self->hover_windows->remove(self->hover_windows);
							}
						}
						if (!hovered)
						{
							av_window_p target_parent = target;
							while (target_parent)
							{
								if (av_system_add_hover_info(self, target_parent, event.mouse_x, event.mouse_y))
								{
									event.type = AV_EVENT_MOUSE_ENTER;
									av_system_bubble_event(self, target_parent, &event);
								}
								target_parent = target_parent->get_parent(target_parent);
							}
						}
						/*target = AV_NULL; */ /* skip event bubble */
						event.type = event_type;
					}
				}
			}
			break;
			case AV_EVENT_KEYBOARD:
			{
				target = self->focus;
			}
			break;
			case AV_EVENT_UPDATE:
				if (event.flags != AV_EVENT_FLAGS_NONE)
				{
					/* FIXME: use event.data pointer or not */
				}
				self->update(self);
				return AV_TRUE;
			break;
			case AV_EVENT_USER:
				target = (av_window_p)event.window;
			break;
			default:break;
		}

		if (target/* && (!self->modal || self->modal == target ||
					   target->is_parent(target, self->modal))*/)
		{
			av_system_bubble_event(self, target, &event);
		}
	}
	return (AV_OK == self->update(self)) && AV_EVENT_QUIT != event.type;
	 /* && (AV_EVENT_KEYBOARD != event.type || event.key != AV_KEY_Escape); */
}

/* FIXME: doesn't work */
static int av_system_flush_events(av_system_p self)
{
	av_input_p input;
	int count;
	if (AV_OK != self->get_input(self, &input))
		return 0;

	/* flush all pending events */
	count = input->flush_events(input);
	return count;
}

static void av_system_loop(av_system_p self)
{
	while (self->step(self));
}

static void av_system_destructor(void* psystem)
{
	av_system_p self = (av_system_p)psystem;
	if (self->graphics)
		O_destroy(self->graphics);

	if (self->root)
		O_destroy(self->root);

	av_torb_service_release("log");
	self->invalid_rects->remove_all(self->invalid_rects, free);
	self->invalid_rects->destroy(self->invalid_rects);
	self->hover_windows->remove_all(self->hover_windows, free);
	self->hover_windows->destroy(self->hover_windows);
}

static av_result_t av_system_constructor(av_object_p object)
{
	av_result_t rc;
	av_system_p self        = (av_system_p)object;
	self->audio             = AV_NULL;
	self->video             = AV_NULL;
	self->input             = AV_NULL;
	self->timer             = AV_NULL;
	self->graphics          = AV_NULL;
	self->graphics_surface  = AV_NULL;
	self->root              = AV_NULL;
	self->focus             = AV_NULL;
	self->capture           = AV_NULL;
	self->modal             = AV_NULL;
	self->modal_result_code = 0;
	self->xbase             = 0;
	self->ybase             = 0;

	if (AV_OK != (rc = av_list_create(&self->hover_windows)))
		return rc;

	if (AV_OK != (rc = av_list_create(&self->invalid_rects)))
	{
		self->hover_windows->destroy(self->hover_windows);
		return rc;
	}

	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&_log)))
	{
		self->hover_windows->destroy(self->hover_windows);
		self->invalid_rects->destroy(self->invalid_rects);
		return rc;
	}

	self->get_audio            = av_system_get_audio;
	self->get_video            = av_system_get_video;
	self->get_input            = av_system_get_input;
	self->get_timer            = av_system_get_timer;
	self->get_graphics         = av_system_get_graphics;
	self->create_window        = av_system_create_window;
	self->removing_window      = av_system_removing_window;
	self->invalidate_rect      = av_system_invalidate_rect;
	self->invalidate_all       = av_system_invalidate_all;
	self->modal_enter          = av_system_modal_enter;
	self->modal_exit           = av_system_modal_exit;
	self->update               = av_system_update;
	self->set_base_resolution  = av_system_set_base_resolution;
	self->get_base_resolution  = av_system_get_base_resolution;
	self->set_capture          = av_system_set_capture;
	self->set_focus            = av_system_set_focus;
	self->is_focus             = av_system_is_focus;
	self->step                 = av_system_step;
	self->flush_events         = av_system_flush_events;
	self->loop                 = av_system_loop;

	return AV_OK;
}

/* Registers system class into TORBA */
av_result_t av_system_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_sound_register_torba()))
		return rc;
	if (AV_OK != (rc = av_audio_register_torba()))
		return rc;
	if (AV_OK != (rc = av_video_register_torba()))
		return rc;
	if (AV_OK != (rc = av_input_register_torba()))
		return rc;
	if (AV_OK != (rc = av_timer_register_torba()))
		return rc;

	return av_torb_register_class("system", "service",
								  sizeof(av_system_t),
								  av_system_constructor, av_system_destructor);
}
