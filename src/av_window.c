
/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_window.c                                        */
/* Description:   Window Manager                                     */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <string.h>
#include <av_log.h>
#include <av_window.h>
#include <av_system.h>
#include <av_graphics.h>

typedef struct _window_ctx_t
{
	av_rect_t             rect;
	av_list_p             children;
	av_window_p           parent;
	av_bool_t             visible;
	av_bool_t             clip_children;
	int                   last_default_pos;
	av_graphics_surface_p graphics_surface;
	av_video_surface_p    video_surface;
	av_log_p              log;
	av_bool_t             painted;
} window_ctx_t, *window_ctx_p;

/* Set a parent to the window */
static av_result_t av_window_set_parent(av_window_p self, av_window_p parent)
{
	window_ctx_p ctx = (window_ctx_p)(self->context);
	av_assert(ctx, "window is not properly initialized");
	av_assert(!(parent && parent->is_parent(parent, self)), "cyclic window inheritance");

	if (ctx->parent) /* not a root window */
	{
		/* remove this window from parent's children */
		ctx->parent->remove_child(ctx->parent, self);
	}

	if (parent)
	{
		/* add window to parent's children */
		return parent->add_child_top(parent, self);
	}

	return AV_OK;
}

/* Checks if a given window is direct or indirect parent to this window */
static av_bool_t av_window_is_parent(av_window_p self, av_window_p parent)
{
	av_window_p aparent = self;
	while (aparent)
	{
		if (aparent == parent)
			return AV_TRUE;
		aparent = aparent->get_parent(aparent);
	}
	return AV_FALSE;
}

/* Returns the direct parent of this window or AV_NULL if the window is root */
static av_window_p av_window_get_parent(av_window_p self)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");

	return ctx->parent;
}

static av_list_p av_window_get_children(av_window_p self)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");

	return ctx->children;
}

/* Adds a window as a child to this window on top or bottom according istop parameter */
static av_result_t av_window_add_child(av_window_p self, av_window_p child, av_bool_t istop)
{
	av_result_t rc;
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx && ctx->children, "window is not properly initialized");
	av_assert(child, "NULL child is not allowed");

	/* detach child from its parent */
	child->detach(child);

	/* add child to self children */
	rc = (istop ? ctx->children->push_last(ctx->children, child)
				: ctx->children->push_first(ctx->children, child));

	if (AV_OK == rc)
	{
		window_ctx_p cctx = (window_ctx_p)(child->context); /* child context */

		/* set self parent to the new child */
		cctx->parent = self;
	}

	return rc;
}

/* Adds a window as a child to this window on top of the other window's children */
static av_result_t av_window_add_child_top(av_window_p self, av_window_p child)
{
	return av_window_add_child(self, child, AV_TRUE);
}

/* Adds a window as a child to this window to bottom of the other window's children */
static av_result_t av_window_add_child_bottom(av_window_p self, av_window_p child)
{
	return av_window_add_child(self, child, AV_FALSE);
}

/*
* Removes a child from the window's children list. 
* \c returns AV_TRUE if the child is found between children of this window and removed
* 		   otherwise AV_FALSE when the child is not found
*/
static av_bool_t av_window_remove_child(av_window_p self, av_window_p child)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	window_ctx_p cctx;
	av_assert(ctx && ctx->children, "window is not properly initialized");
	av_assert(child, "NULL child is not allowed");
	cctx = (window_ctx_p)child->context;

	if (child->get_parent(child) != self) return AV_FALSE; /* self is not direct parent of the child */

	/* search child in self children */
	for (ctx->children->first(ctx->children);
		 ctx->children->has_more(ctx->children);
		 ctx->children->next(ctx->children))
	{
		if (child == ctx->children->get(ctx->children)) break; /* child found in self children */
	}
	av_assert(child == ctx->children->get(ctx->children), "child is not found between children of its parent");

	ctx->children->remove(ctx->children);
	cctx->parent = AV_NULL; /* removed child has no parent */

	return AV_TRUE;
}

/* Sets window's visibility status */
static void av_window_set_visible(av_window_p self, av_bool_t visible)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_bool_t prevvisible;

	av_assert(ctx, "window is not properly initialized");
	prevvisible = self->is_visible(self);
	ctx->visible = visible;
	
	if (self->is_visible(self) != prevvisible)
	{
		/*self->update(self, AV_UPDATE_INVALIDATE);*/
		av_rect_t rect;
		self->get_rect(self, &rect);
		rect.x = rect.y = 0;
		self->rect_absolute(self, &rect);
		av_assert(self->system, "System is not initialized yet");
		if (rect.w && rect.h)
			self->system->invalidate_rect(self->system, &rect);
	}
}

/* Returns the window's visibility status */
static av_bool_t av_window_is_visible(av_window_p self)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");

	/* a window is visible if all its parents are visible */
	if (ctx->parent)
		return ctx->visible && ctx->parent->is_visible(ctx->parent);
	else
		return ctx->visible;
}

/* Tells this window to clip its children or not */
static void av_window_set_clip_children(av_window_p self, av_bool_t clipped)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");
	ctx->clip_children = clipped;
}

/* Return AV_TRUE if the window is clipping its children, AV_FALSE otherwise */
static av_bool_t av_window_are_children_clipped(av_window_p self)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");
	return ctx->clip_children;
}

/* Returns the window's rectangle */
static void av_window_get_rect(av_window_p self, av_rect_p rect)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx, "window is not properly initialized");
	av_rect_copy(rect, &ctx->rect);
/*	if (ctx->parent)
	{
		rect->x += ctx->parent->origin_x;
		rect->y += ctx->parent->origin_y;
	}*/
}

/* Repaints graphics buffer */
static av_result_t av_window_repaint(av_window_p self)
{
	if (self->on_paint)
	{
		av_graphics_p graphics;
		av_result_t rc;
		av_rect_t rect;
		window_ctx_p ctx = (window_ctx_p)self->context;
		av_system_p sys = (av_system_p)self->system;
	
		self->get_rect(self, &rect);
		if (!rect.w || !rect.h)
			return AV_EARG;
	
		rect.x = rect.y = 0;
		if (AV_OK != (rc = sys->get_graphics(sys, &graphics)))
		{
			ctx->log->error(ctx->log, "av_window.c:%d Unable to obtain graphics object (rc = %d)", __LINE__, rc);
			return rc;
		}
		
		/* prepare graphics buffer */
		if (AV_NULL == ctx->graphics_surface)
		{
			av_video_p video;
	
			/* create new graphics buffer */
			sys->get_video(sys, &video);
			if (AV_OK != (rc = video->create_surface(video, rect.w, rect.h, &ctx->video_surface)))
			{
				ctx->log->error(ctx->log, "av_window.c:%d Unable to create graphics surface (rc = %d)", __LINE__, rc);
				return rc;
			}
			
			if (AV_OK != (rc = graphics->wrap_surface(graphics, (av_surface_p)ctx->video_surface, &ctx->graphics_surface)))
			{
				ctx->log->error(ctx->log, "av_window.c:%d Unable to wrap video surface (rc = %d)", __LINE__, rc);
				return rc;
			}
		}
		else
		{
			int gw, gh;
			((av_surface_p)ctx->graphics_surface)->get_size((av_surface_p)ctx->graphics_surface, &gw, &gh);
			if (gw != rect.w || gh != rect.h)
			{
				/* resize graphics buffer */
				if (AV_OK != (rc = ((av_surface_p)ctx->graphics_surface)->set_size((av_surface_p)ctx->graphics_surface, rect.w, rect.h)))
				{
					ctx->log->error(ctx->log, "av_window.c:%d Unable to resize graphics buffer (rc = %d)", __LINE__, rc);
					return rc;
				}
			}
		}

		ctx->video_surface->fill_rect(ctx->video_surface, &rect, 0, 0, 0, 0);
		graphics->begin(graphics, ctx->graphics_surface);
		graphics->set_clip(graphics, &rect);
		self->on_paint(self, graphics);
		graphics->end(graphics);
	}
	return AV_OK;
}

static void av_window_update(av_window_p self, av_window_update_t window_update)
{
	av_bool_t isvisible = self->is_visible(self);
	if (isvisible || AV_UPDATE_REPAINT == window_update)
	{
		window_ctx_p ctx = (window_ctx_p)self->context;
		av_rect_t rect;
		if (AV_UPDATE_REPAINT == window_update || !ctx->painted)
		{
			av_window_repaint(self);
			ctx->painted = AV_TRUE;
		}

		if (isvisible)
		{
			self->get_rect(self, &rect);
			rect.x = rect.y = 0;
			self->rect_absolute(self, &rect);
			av_assert(self->system, "System is not initialized yet");
			if (rect.w && rect.h)
				self->system->invalidate_rect(self->system, &rect);
		}
	}
}

/* Gets window graphics buffer */
static av_result_t av_window_get_surface(av_window_p self, av_surface_p* ppsurface)
{
	window_ctx_p ctx = (window_ctx_p)self->context;
	*ppsurface = (av_surface_p)ctx->video_surface;
	return AV_OK;
}

/* Raises or lower window according israise parameter */
static av_result_t av_window_raise_lower(av_window_p self, av_bool_t israise)
{
	av_result_t rc = AV_OK;
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_window_p parent;
	av_assert(ctx, "window is not properly initialized");

	parent = ctx->parent;
	if (parent)
	{
		/* reattach the window on top or on the bottom */
		self->detach(self);
		if (israise)
			rc = parent->add_child_top(parent, self);
		else
			rc = parent->add_child_bottom(parent, self);
		
		if (AV_OK == rc)
			self->update(self, AV_UPDATE_INVALIDATE);
	}
	return rc;
}

/* Raises this window on top of the other sibling windows */
static av_result_t av_window_raise_top(av_window_p self)
{
	return av_window_raise_lower(self, AV_TRUE);
}

/* Lower this window beneath the other sibling windows */
static av_result_t av_window_lower_bottom(av_window_p self)
{
	return av_window_raise_lower(self, AV_FALSE);
}

/* Detach this window from its parent and siblings. If the window is a root then do nothing */
static void av_window_detach(av_window_p self)
{
	av_window_p parent;

	parent = self->get_parent(self);
	if (parent)
	{
		/* self window is no longer child to his parent  */
		parent->remove_child(parent, self);
	}
}

/* Window destructor */
static void av_window_destructor(void* pobject)
{
	av_window_p self = (av_window_p)pobject;
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_system_p sys = (av_system_p)self->system;
	av_list_p children;
	av_assert(ctx && ctx->children, "window is not properly initialized");
	sys->removing_window(sys, self);
	children = ctx->children;
	self->detach(self);
	while (children->size(children) > 0)
	{
		av_window_p child;
		children->last(children);
		child = (av_window_p)children->get(children);
		O_destroy(child);
	}
	children->destroy(children);
	
	if (ctx->graphics_surface)
		O_destroy(ctx->graphics_surface);
		
	if (ctx->video_surface)
		O_destroy(ctx->video_surface);

	av_torb_service_release("log");
	free(ctx);
}

/* Windows managment algorithms */

/* Gets the root window from the tree a given window is a part */

/* FIXME: unused
static av_window_p av_window_get_root(av_window_p self)
{
	if (self->get_parent(self))
		return av_window_get_root(self->get_parent(self));
	else
		return self;
}
*/

/*
* Converts a given rect in absolute coordinates according the root window from the tree
* the target window belongs to. The specified rect is assumed to be given according the
* target window as an origin.
*/
static void av_window_rect_absolute(av_window_p self, av_rect_p rect)
{
	av_window_p parent = self->get_parent(self);
	av_rect_t arect;
	self->get_rect(self, &arect);
	av_rect_move(rect, arect.x, arect.y);
	while (parent)
	{
		parent->get_rect(parent, &arect);
		av_rect_move(rect, arect.x + parent->origin_x, arect.y + parent->origin_y);
		parent = parent->get_parent(parent);
	}
}

/*
* Clip rect with all parents storing the result to cliprect
* Returns AV_TRUE if the output cliprect is valid rectangle, AV_FALSE otherwise
* Rectangles must be given according to a same origine (e.g. in absolute coordinates)
*/
static av_bool_t av_window_clip_with_parents(av_window_p parent, av_rect_p rect, av_rect_p cliprect)
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

static av_bool_t av_window_point_inside(av_window_p self, int x, int y)
{
	av_window_p parent;
	av_rect_t rect;
	self->get_rect(self, &rect);
	rect.x = rect.y = 0;
	self->rect_absolute(self, &rect);
	
	if ((parent = self->get_parent(self)))
	{
		if (parent->point_inside(parent, x, y))
			av_window_clip_with_parents(parent, &rect, &rect);
		else
			return AV_FALSE;
	}

	return av_rect_point_inside(&rect, x, y);
}

static void av_window_manage_rect(av_window_p self, av_rect_p rect)
{
	window_ctx_p ctx = (window_ctx_p)(self->context);
	av_assert(ctx, "window is not properly initialized");

	if (rect->w == AV_DEFAULT)
	{
		if (rect->h != AV_DEFAULT)
			rect->w = 2*rect->h/3;
		else
			rect->w = 2*ctx->rect.w/3;
	}

	if (rect->h == AV_DEFAULT)
		rect->h = 2*rect->w/3;

	if (rect->x == AV_DEFAULT || rect->y == AV_DEFAULT)
	{
		switch (ctx->last_default_pos)
		{
			case 0: /* top/right */
				rect->y = 0;
				rect->x = ctx->rect.w - rect->w;
			break;
			case 1: /* bottom/left */
				rect->y = ctx->rect.h - rect->h;
				rect->x = 0;
			break;
			case 2: /* top/left */
				rect->y = 0;
				rect->x = 0;
			break;
			case 3: /* bottom/right */
				rect->y = ctx->rect.h - rect->h;
				rect->x = ctx->rect.w - rect->w;
			break;
		}
		ctx->last_default_pos = (ctx->last_default_pos + 1) % 4;
	}
}

/* Sets new window rectangle */
static av_result_t av_window_set_rect(av_window_p self, av_rect_p newrect)
{
	av_rect_t oldrect;
	window_ctx_p ctx = (window_ctx_p)(self->context);
	av_list_p children = self->get_children(self);
	av_window_p parent =          /* parent window */
		self->get_parent(self);

	if (parent)
		av_window_manage_rect(parent, newrect);

	self->get_rect(self, &oldrect);
	if (av_rect_compare(newrect, &oldrect))
	{
		/* window position is not changed. nothing happens */
		return AV_OK;
	}

	if (!parent)
	{
		/* set newrect to the root window */
		av_rect_copy(&ctx->rect, newrect);
		self->update(self, AV_UPDATE_REPAINT);
		return AV_OK;
	}

	/* invalidate old window rect */
	self->update(self, AV_UPDATE_INVALIDATE);

	/* if children not clipped, change their positions */
	if (AV_FALSE == self->are_children_clipped(self))
	{
		/* move children to keep their absolute coordinates unchanged */
		for (children->first(children); children->has_more(children); children->next(children))
		{
			av_rect_t newcrect;
			av_window_p child = (av_window_p)children->get(children);
			child->get_rect(child, &newcrect);
			newcrect.x -= (newrect->x - oldrect.x);
			newcrect.y -= (newrect->y - oldrect.y);
			child->set_rect(child, &newcrect);
		}
	}

	/* invalidate new window rect */
	av_rect_copy(&ctx->rect, newrect);
	if (oldrect.w != newrect->w || oldrect.h != newrect->h)
	{
		/* set new window rect */
		self->update(self, AV_UPDATE_REPAINT);
	}
	else
	{
		/* set new window rect */
		self->update(self, AV_UPDATE_INVALIDATE);
	}

	return AV_OK;
}

static av_window_p av_window_get_child_xy(av_window_p self, int x, int y)
{
	av_list_p children;
	window_ctx_p ctx = (window_ctx_p)self->context;
	av_assert(ctx && ctx->children, "window is not properly initialized");
	children = ctx->children;
	
	for (children->last(children); children->has_more(children); children->prev(children))
	{
		av_window_p child = (av_window_p)children->get(children);
		if (child->is_visible(child))
		{
			av_rect_t crect;
			child->get_rect(child, &crect);
			if (av_rect_point_inside(&crect, x, y))
				return child;
		}
	}
	return AV_NULL;
}

static void av_window_capture(av_window_p self)
{
	av_system_p sys = (av_system_p)self->system;
	av_assert(sys, "context can't be NULL");
	sys->set_capture(sys, self);
}

static void av_window_uncapture(av_window_p self)
{
	av_system_p sys = (av_system_p)self->system;
	av_assert(sys, "context can't be NULL");
	sys->set_capture(sys, AV_NULL);
}

static void av_window_scroll(av_window_p self, int x, int y)
{
	self->origin_x = x;
	self->origin_y = y;
	self->update(self, AV_UPDATE_INVALIDATE);
}

static void av_window_set_cursor(av_window_p self, av_video_cursor_shape_t cursor)
{
	self->cursor = cursor;
}

static av_video_cursor_shape_t av_window_get_cursor(av_window_p self)
{
	return self->cursor;
}

static void av_window_set_cursor_visible(av_window_p self, av_bool_t visible)
{
	self->cursor_visible = visible;
}

static av_bool_t av_window_is_cursor_visible(av_window_p self)
{
	return self->cursor_visible;
}

static void av_window_set_hover_delay(av_window_p  self, unsigned long hover_delay)
{
	self->hover_delay = hover_delay;
}

static unsigned long av_window_get_hover_delay(av_window_p self)
{
	return self->hover_delay;
}

static void av_window_set_bubble_events(av_window_p self, av_bool_t bubble_events)
{
	self->bubble_events = bubble_events;
}

static av_bool_t av_window_is_bubble_events(av_window_p self)
{
	return self->bubble_events;
}

static void av_window_set_handle_events(av_window_p self, av_bool_t handle_events)
{
	self->handle_events = handle_events;
}

static av_bool_t av_window_is_handle_events(av_window_p self)
{
	return self->handle_events;
}

static av_bool_t av_window_on_mouse(av_window_p self, int x, int y)
{
	AV_UNUSED(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	return AV_FALSE;
}

static av_bool_t av_window_on_mouse_button(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	AV_UNUSED(self);
	AV_UNUSED(button);
	AV_UNUSED(x);
	AV_UNUSED(y);
	return AV_FALSE;
}

static av_bool_t av_window_on_key(av_window_p self, av_key_t key)
{
	AV_UNUSED(self);
	AV_UNUSED(key);
	return AV_FALSE;
}

static av_bool_t av_window_on_focus(av_window_p self, av_bool_t isfocus)
{
	AV_UNUSED(self);
	AV_UNUSED(isfocus);
	return AV_FALSE;
}

static av_bool_t av_window_on_user(av_window_p self, int id, void* data)
{
	AV_UNUSED(self);
	AV_UNUSED(id);
	AV_UNUSED(data);
	return AV_FALSE;
}

static av_bool_t av_window_on_paint(av_window_p self, av_graphics_p graphics)
{
	AV_UNUSED(self);
	AV_UNUSED(graphics);
	return AV_FALSE;
}

static av_bool_t av_window_on_update(av_window_p self, av_video_surface_p video_surface, av_rect_p rect)
{
	av_rect_t srcrect;
	window_ctx_p ctx = (window_ctx_p)(self->context);
	av_rect_init(&srcrect, 0, 0, rect->w, rect->h);
	self->rect_absolute(self, &srcrect);
	srcrect.x = rect->x - srcrect.x;
	srcrect.y = rect->y - srcrect.y;
	if (ctx->video_surface)
		video_surface->blit(video_surface, rect, (av_surface_p)ctx->video_surface, &srcrect);
	return AV_TRUE;
}

static av_bool_t av_window_on_event(av_window_p self, av_event_p event)
{
	switch (event->type)
	{
		case AV_EVENT_MOUSE_MOTION:
				return self->on_mouse_move(self, event->mouse_x, event->mouse_y);
		case AV_EVENT_MOUSE_ENTER:
				return self->on_mouse_enter(self, event->mouse_x, event->mouse_y);
		case AV_EVENT_MOUSE_HOVER:
				return self->on_mouse_hover(self, event->mouse_x, event->mouse_y);
		case AV_EVENT_MOUSE_LEAVE:
				return self->on_mouse_leave(self, event->mouse_x, event->mouse_y);
		case AV_EVENT_MOUSE_BUTTON:
		{
			int x = event->mouse_x;
			int y = event->mouse_y;
			if (AV_BUTTON_PRESSED == event->button_status)
				return self->on_mouse_button_down(self, event->mouse_button, x, y);
			else
				return self->on_mouse_button_up(self, event->mouse_button, x, y);
		}
		break;
		case AV_EVENT_KEYBOARD:
		{
			if (AV_BUTTON_PRESSED == event->button_status)
				return self->on_key_down(self, event->key);
			else
				return self->on_key_up(self, event->key);
		}
		break;
		case AV_EVENT_FOCUS:
			return self->on_focus(self, (av_bool_t)(event->data));
		break;
		case AV_EVENT_USER:
			return self->on_user(self, event->user_id, event->data);
		break;
		default:break;
	}
	return AV_FALSE;
}

/* Initializes memory given by the input pointer with the window's class information */
static av_result_t av_window_constructor(av_object_p object)
{
	av_result_t rc;
	window_ctx_p ctx;
	av_window_p self = (av_window_p)object;

	/* initializes self */
	ctx = calloc(1, sizeof(window_ctx_t));
	if (!ctx) return AV_EMEM;
	if (AV_OK != (rc = av_list_create(&ctx->children)))
	{
		free(ctx);
		return AV_EMEM;
	}
	ctx->visible                = AV_TRUE; /* invisible by default */
	ctx->clip_children          = AV_TRUE; /* clip children by default */
	self->origin_x              = self->origin_y = 0;
	self->hover_delay           = 0;
	self->cursor                = AV_VIDEO_CURSOR_DEFAULT;
	self->cursor_visible        = AV_TRUE;
	self->bubble_events         = AV_TRUE;
	self->handle_events         = AV_TRUE;
	self->context               = ctx;
	self->data                  = AV_NULL;
	self->dispatcher            = AV_NULL;
	self->set_parent            = av_window_set_parent;
	self->get_parent            = av_window_get_parent;
	self->get_children          = av_window_get_children;
	self->is_parent             = av_window_is_parent;
	self->add_child_top         = av_window_add_child_top;
	self->add_child_bottom      = av_window_add_child_bottom;
	self->remove_child          = av_window_remove_child;
	self->set_visible           = av_window_set_visible;
	self->is_visible            = av_window_is_visible;
	self->set_clip_children     = av_window_set_clip_children;
	self->are_children_clipped  = av_window_are_children_clipped;
	self->get_rect              = av_window_get_rect;
	self->set_rect              = av_window_set_rect;
	self->rect_absolute         = av_window_rect_absolute;
	self->point_inside          = av_window_point_inside;
	self->update                = av_window_update;
	self->get_surface           = av_window_get_surface;
	self->raise_top             = av_window_raise_top;
	self->lower_bottom          = av_window_lower_bottom;
	self->detach                = av_window_detach;
	self->get_child_xy          = av_window_get_child_xy;
	self->capture               = av_window_capture;
	self->uncapture             = av_window_uncapture;
	self->scroll                = av_window_scroll;
	self->set_cursor            = av_window_set_cursor;
	self->get_cursor            = av_window_get_cursor;
	self->set_cursor_visible    = av_window_set_cursor_visible;
	self->is_cursor_visible     = av_window_is_cursor_visible;
	self->set_hover_delay       = av_window_set_hover_delay;
	self->get_hover_delay       = av_window_get_hover_delay;
	self->set_bubble_events     = av_window_set_bubble_events;
	self->is_bubble_events      = av_window_is_bubble_events;
	self->set_handle_events     = av_window_set_handle_events;
	self->is_handle_events      = av_window_is_handle_events;
	self->on_event             = av_window_on_event;
	self->on_mouse_move        = av_window_on_mouse;
	self->on_mouse_enter       = av_window_on_mouse;
	self->on_mouse_hover       = av_window_on_mouse;
	self->on_mouse_leave       = av_window_on_mouse;
	self->on_mouse_button_up   = av_window_on_mouse_button;
	self->on_mouse_button_down = av_window_on_mouse_button;
	self->on_key_up            = av_window_on_key;
	self->on_key_down          = av_window_on_key;
	self->on_focus             = av_window_on_focus;
	self->on_user              = av_window_on_user;
	self->on_paint             = av_window_on_paint;
	self->on_update            = av_window_on_update;
	
	if (AV_OK != (rc = av_torb_service_addref("log", (av_service_p*)&ctx->log)))
	{
		free(ctx);
		return rc;
	}

	return AV_OK;
}

av_result_t av_window_register_torba( void )
{
	return av_torb_register_class("window", AV_NULL, sizeof(av_window_t),
								  av_window_constructor, av_window_destructor);
}
