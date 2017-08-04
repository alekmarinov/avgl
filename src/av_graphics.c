/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics.c                                      */
/* Description:   Abstract graphics class                            */
/*                                                                   */
/*********************************************************************/

#include <av_video.h>
#include <av_graphics.h>
#include <av_prefs.h>

/* Create graphics surface */
static av_result_t av_graphics_create_surface(av_graphics_p self, int width, int height, av_graphics_surface_p *ppsurface)
{
	AV_UNUSED(self);
	AV_UNUSED(width);
	AV_UNUSED(height);
	AV_UNUSED(ppsurface);
	return AV_ESUPPORTED;
}

/* Create graphics surface from file */
static av_result_t av_graphics_create_surface_file(av_graphics_p self, const char* filename, av_graphics_surface_p *ppsurface)
{
	AV_UNUSED(self);
	AV_UNUSED(filename);
	AV_UNUSED(ppsurface);
	return AV_ESUPPORTED;
}

/* Create graphics surface from file */
static av_result_t av_graphics_save_surface_file(av_graphics_p self, av_graphics_surface_p surface, const char* filename)
{
	AV_UNUSED(self);
	AV_UNUSED(surface);
	AV_UNUSED(filename);
	return AV_ESUPPORTED;
}

/* Wrap graphics surface */
static av_result_t av_graphics_wrap_surface(av_graphics_p self, av_surface_p wrapsurface, av_graphics_surface_p *ppsurface)
{
	AV_UNUSED(self);
	AV_UNUSED(wrapsurface);
	AV_UNUSED(ppsurface);
	return AV_ESUPPORTED;
}

/* Setup drawing environment on a target surface */
static av_result_t av_graphics_begin(av_graphics_p self, av_graphics_surface_p surface)
{
	AV_UNUSED(self);
	AV_UNUSED(surface);
	return AV_ESUPPORTED;
}

static av_result_t av_graphics_get_target_surface(av_graphics_p self,
												  av_graphics_surface_p* ppgraphicssurface)
{
	AV_UNUSED(self);
	AV_UNUSED(ppgraphicssurface);
	return AV_ESUPPORTED;
}

/* Flushes collected drawing data on the target surface given with the method \c begin */
static void av_graphics_end(av_graphics_p self)
{
	AV_UNUSED(self);
}

/* Set clipping rectangle */
static av_result_t av_graphics_set_clip(av_graphics_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
}

/* Get clipping rectangle */
static av_result_t av_graphics_get_clip(av_graphics_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
}

/* Saves the current graphics state in a stack which can be restored later via \c restore */
static av_result_t av_graphics_save(av_graphics_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/* Restores the last saved state via \c save from a stack and makes it active for later drawing methods. */
static av_result_t av_graphics_restore(av_graphics_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/* Sets new position (x, y) of the current point */
static void av_graphics_move_to(av_graphics_p self, double x, double y)
{
	AV_UNUSED(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
}

/*
*	Begin a new sub-path. After this call the current point will offset
*	by (\c dx, \c dy).
*
*	Given a current point of (x, y), rel_move_to(dx, dy)
*	is logically equivalent to move_to (x + dx, y + dy).
*/
static void av_graphics_rel_move_to(av_graphics_p self, double dx, double dy)
{
	AV_UNUSED(self);
	AV_UNUSED(dx);
	AV_UNUSED(dy);
}

/*
*	Adds a line to the path from the current point to position (\c x, \c y)
*	in user-space coordinates. After this call the current point
*	will be (\c x, \c y)
*/
static av_result_t av_graphics_line_to(av_graphics_p self, double x, double y)
{
	AV_UNUSED(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	return AV_ESUPPORTED;
}

/*
*	Relative-coordinate version of \c line_to. Adds a line to the
*	path from the current point to a point that is offset from the
*	current point by (\c dx, \c dy) in user space. After this call the
*	current point will be offset by (\c dx, \c dy).
*/
static av_result_t av_graphics_rel_line_to(av_graphics_p self, double dx, double dy)
{
	AV_UNUSED(self);
	AV_UNUSED(dx);
	AV_UNUSED(dy);
	return AV_ESUPPORTED;
}

/*
*	Adds a cubic Bézier spline to the path from the current point to
*	position (\c x3, \c y3) in user-space coordinates, using (\c x1, \c y1) and
*	(\c x2, \c y2) as the control points. After this call the current point
*	will be (\c x3, \c y3).
*/
static av_result_t av_graphics_curve_to(av_graphics_p self,
							double x1_, double y1_,
							double x2_, double y2_,
							double x3_, double y3_)
{
	AV_UNUSED(self);
	AV_UNUSED(x1_);
	AV_UNUSED(x2_);
	AV_UNUSED(x3_);
	AV_UNUSED(y1_);
	AV_UNUSED(y2_);
	AV_UNUSED(y3_);
	return AV_ESUPPORTED;
}

/*
*	Relative-coordinate version of \c curve_to. All offsets are
*	relative to the current point. Adds a cubic Bézier spline to the
*	path from the current point to a point offset from the current
*	point by (\c dx3, \c dy3), using points offset by (\c dx1, \c dy1) and
*	(\c dx2, \c dy2) as the control points. After this call the current
*	point will be offset by (\c dx3, \c dy3).
*/
static av_result_t av_graphics_rel_curve_to(av_graphics_p self,
								double dx1, double dy1,
								double dx2, double dy2,
								double dx3, double dy3)
{
	AV_UNUSED(self);
	AV_UNUSED(dx1);
	AV_UNUSED(dx2);
	AV_UNUSED(dx3);
	AV_UNUSED(dy1);
	AV_UNUSED(dy2);
	AV_UNUSED(dy3);
	return AV_ESUPPORTED;
}

/*
*	Adds a circular arc of the given \c radius to the current path.  The
*	arc is centered at (\c xc, \c yc), begins at \c start_angle and proceeds in
*	the direction of increasing angles to end at \c end_angle. If \c end_angle is
*	less than \c start_angle it will be progressively increased by 2*M_PI
*	until it is greater than \c start_angle
*
*	If there is a current point, an initial line segment will be added
*	to the path to connect the current point to the beginning of the
*	arc.
*
*	Angles are measured in radians. An angle of 0.0 is in the direction
*	of the positive X axis (in user space). An angle of M_PI/2.0 radians
*	(90 degrees) is in the direction of the positive Y axis (in
*	user space). Angles increase in the direction from the positive X
*	axis toward the positive Y axis. So with the default transformation
*	matrix, angles increase in a clockwise direction.
*
*	This function gives the arc in the direction of increasing angles;
*	\see arc_negative to get the arc in the direction of decreasing angles.
*
*	The arc is circular in user space. To achieve an elliptical arc,
*	you can scale the current transformation matrix by different
*	amounts in the X and Y directions.
*
*	\arg xc coordinate to the arc circle center
*	\arg yc coordinate to the arc circle center
*	\arg radius of the arc circle
*	\arg start_angle of the arc
*	\arg end_angle of the arc
*/
static av_result_t av_graphics_arc(av_graphics_p self,
					double cx,
					double cy,
					double radius,
					double start_angle,
					double end_angle)
{
	AV_UNUSED(self);
	AV_UNUSED(cx);
	AV_UNUSED(cy);
	AV_UNUSED(radius);
	AV_UNUSED(start_angle);
	AV_UNUSED(end_angle);
	return AV_ESUPPORTED;
}

/*
*	Adds a circular arc of the given \c radius to the current path.  The
*	arc is centered at (\c xc, \c yc), begins at \c start_angle and proceeds in
*	the direction of decreasing angles to end at \c end_angle. If \c end_angle is
*	greater than \c start_angle it will be progressively decreased by 2*M_PI
*	until it is less than \c start_angle
*
*	\c see arc for more details. This function differs only in the
*	direction of the arc between the two angles.
*
*	\arg xc coordinate to the arc circle center
*	\arg yc coordinate to the arc circle center
*	\arg radius of the arc circle
*	\arg start_angle of the arc
*	\arg end_angle of the arc
*/
static av_result_t av_graphics_arc_negative(av_graphics_p self,
					double cx,
					double cy,
					double radius,
					double start_angle,
					double end_angle)
{
	AV_UNUSED(self);
	AV_UNUSED(cx);
	AV_UNUSED(cy);
	AV_UNUSED(radius);
	AV_UNUSED(start_angle);
	AV_UNUSED(end_angle);
	return AV_ESUPPORTED;
}

/*
*	Adds a line segment to the path from the current point to the
*	beginning of the current sub-path
*/
static av_result_t av_graphics_close_path(av_graphics_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/*
*	Adds a closed sub-path rectangle of the given size to the current
*	path at position (x, y)
*/
static av_result_t av_graphics_rectangle(av_graphics_p self, av_rect_p rect)
{
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
}

/* Sets line width */
static av_result_t av_graphics_set_line_width(av_graphics_p self, double line_width)
{
	AV_UNUSED(self);
	AV_UNUSED(line_width);
	return AV_ESUPPORTED;
}

/* Sets line cap */
static av_result_t av_graphics_set_line_cap(av_graphics_p self, av_line_cap_t cap)
{
	AV_UNUSED(self);
	AV_UNUSED(cap);
	return AV_ESUPPORTED;
}

/* Sets line join */
static av_result_t av_graphics_set_line_join(av_graphics_p self, av_line_join_t join)
{
	AV_UNUSED(self);
	AV_UNUSED(join);
	return AV_ESUPPORTED;
}

/* Draw contours around the painted objects */
static av_result_t av_graphics_stroke(av_graphics_p self, av_bool_t preserve)
{
	AV_UNUSED(self);
	AV_UNUSED(preserve);
	return AV_ESUPPORTED;
}

/* A drawing operator that fills the current path according to the current fill rule */
static av_result_t av_graphics_fill(av_graphics_p self, av_bool_t preserve)
{
	AV_UNUSED(self);
	AV_UNUSED(preserve);
	return AV_ESUPPORTED;
}

/* A drawing operator that fills the current path according to the current fill rule */
static av_result_t av_graphics_flush(av_graphics_p self)
{
	AV_UNUSED(self);
	return AV_ESUPPORTED;
}

/* Set left/top offset to the target surface becoming (0,0) drawing origin */
static void av_graphics_set_offset(av_graphics_p self, double dx, double dy)
{
	AV_UNUSED(self);
	AV_UNUSED(dx);
	AV_UNUSED(dy);
}

/* Get left/top offset to the target surface becoming (0,0) drawing origin */
static void av_graphics_get_offset(av_graphics_p self, double* dx, double* dy)
{
	AV_UNUSED(self);
	AV_UNUSED(dx);
	AV_UNUSED(dy);
}

/* Scales X,Y user-space axes */
static void av_graphics_set_scale(av_graphics_p self, double sx, double sy)
{
	AV_UNUSED(self);
	AV_UNUSED(sx);
	AV_UNUSED(sy);
}

static void av_graphics_set_color_rgba(av_graphics_p self, double r, double g, double b, double a)
{
	AV_UNUSED(self);
	AV_UNUSED(r);
	AV_UNUSED(g);
	AV_UNUSED(b);
	AV_UNUSED(a);
}

static void av_graphics_show_text(av_graphics_p self, const char* utf8)
{
	AV_UNUSED(self);
	AV_UNUSED(utf8);
}

static void av_graphics_text_path(av_graphics_p self, const char* utf8)
{
	AV_UNUSED(self);
	AV_UNUSED(utf8);
}

static void av_graphics_show_image(av_graphics_p self, double x, double y, av_graphics_surface_p surface)
{
	AV_UNUSED(self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	AV_UNUSED(surface);
}

static av_result_t av_graphics_get_text_extents(av_graphics_p self,
												const char* utf8,
												int* pwidth,
												int* pheight,
												int* pxbearing,
												int* pybearing,
												int* pxadvance,
												int* pyadvance)
{
	AV_UNUSED(self);
	AV_UNUSED(utf8);
	AV_UNUSED(pwidth);
	AV_UNUSED(pheight);
	AV_UNUSED(pxbearing);
	AV_UNUSED(pybearing);
	AV_UNUSED(pxadvance);
	AV_UNUSED(pyadvance);
	return AV_ESUPPORTED;
}

static av_result_t av_graphics_select_font_face(av_graphics_p self, const char* fontface, av_font_slant_t slant, av_font_weight_t weight)
{
	AV_UNUSED(self);
	AV_UNUSED(fontface);
	AV_UNUSED(slant);
	AV_UNUSED(weight);
	return AV_ESUPPORTED;
}

static av_result_t av_graphics_set_font_size(av_graphics_p self, int size)
{
	AV_UNUSED(self);
	AV_UNUSED(size);
	return AV_ESUPPORTED;
}

static void graphics_prefs_observer(void* param, const char* name, av_prefs_value_p value)
{
	av_prefs_p prefs;
	av_graphics_p self = (av_graphics_p)param;
	AV_UNUSED(name);
	AV_UNUSED(value);

	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		int xres, yres;
		int xbase, ybase;
		prefs->get_int(prefs, "system.video.xres", -1, &xres);
		prefs->get_int(prefs, "system.video.yres", -1, &yres);
		if (xres > 0 && yres > 0)
		{
			prefs->get_int(prefs, "graphics.xbase", xres, &xbase);
			prefs->get_int(prefs, "graphics.ybase", yres, &ybase);
			av_torb_service_release("prefs");
			self->scale_x = (double)xres / xbase;
			self->scale_y = (double)yres / ybase;
		}
	}
}

static void av_graphics_destructor(void* pobject)
{
	av_prefs_p prefs;
	AV_UNUSED(pobject);

	/* remove preference observers */
	if (AV_OK == av_torb_service_addref("prefs", (av_service_p*)&prefs))
	{
		prefs->unregister_observer(prefs, graphics_prefs_observer);
		av_torb_service_release("prefs");
	}
}

/* Initializes memory given by the input pointer with the graphics class information */
static av_result_t av_graphics_constructor(av_object_p pobject)
{
	av_result_t rc;
	av_prefs_p prefs;
	av_graphics_p self        = (av_graphics_p)pobject;

	self->scale_x = self->scale_y = 1.0f;

	self->graphics_surface    = AV_NULL;
	self->create_surface      = av_graphics_create_surface;
	self->create_surface_file = av_graphics_create_surface_file;
	self->save_surface_file   = av_graphics_save_surface_file;
	self->wrap_surface        = av_graphics_wrap_surface;
	self->begin               = av_graphics_begin;
	self->get_target_surface  = av_graphics_get_target_surface;
	self->end                 = av_graphics_end;
	self->set_clip            = av_graphics_set_clip;
	self->get_clip            = av_graphics_get_clip;
	self->save                = av_graphics_save;
	self->restore             = av_graphics_restore;
	self->move_to             = av_graphics_move_to;
	self->rel_move_to         = av_graphics_rel_move_to;
	self->line_to             = av_graphics_line_to;
	self->rel_line_to         = av_graphics_rel_line_to;
	self->curve_to            = av_graphics_curve_to;
	self->rel_curve_to        = av_graphics_rel_curve_to;
	self->arc                 = av_graphics_arc;
	self->arc_negative        = av_graphics_arc_negative;
	self->close_path          = av_graphics_close_path;
	self->rectangle           = av_graphics_rectangle;
	self->set_line_width      = av_graphics_set_line_width;
	self->set_line_cap        = av_graphics_set_line_cap;
	self->set_line_join       = av_graphics_set_line_join;
	self->stroke              = av_graphics_stroke;
	self->fill                = av_graphics_fill;
	self->flush               = av_graphics_flush;
	self->set_offset          = av_graphics_set_offset;
	self->get_offset          = av_graphics_get_offset;
	self->set_scale           = av_graphics_set_scale;
	self->set_color_rgba      = av_graphics_set_color_rgba;
	self->text_path           = av_graphics_text_path;
	self->show_text           = av_graphics_show_text;
	self->show_image          = av_graphics_show_image;
	self->get_text_extents    = av_graphics_get_text_extents;
	self->select_font_face    = av_graphics_select_font_face;
	self->set_font_size       = av_graphics_set_font_size;

	/* register preference observers */
	if (AV_OK != (rc = av_torb_service_addref("prefs", (av_service_p*)&prefs)))
	{
		return rc;
	}

	prefs->register_observer(prefs, "graphics.xbase", graphics_prefs_observer, self);
	prefs->register_observer(prefs, "graphics.ybase", graphics_prefs_observer, self);
	prefs->register_observer(prefs, "system.video.xres", graphics_prefs_observer, self);
	prefs->register_observer(prefs, "system.video.yres", graphics_prefs_observer, self);
	graphics_prefs_observer(self, AV_NULL, AV_NULL);
	av_torb_service_release("prefs");

	return AV_OK;
}

av_result_t av_graphics_register_torba(void)
{
	av_result_t rc;
	if (AV_OK != (rc = av_torb_register_class("graphics_surface", "surface",
											  sizeof(av_graphics_surface_t),
											  AV_NULL, AV_NULL)))
	{
		return rc;
	}
	return av_torb_register_class("graphics", AV_NULL, sizeof(av_graphics_t),
								  av_graphics_constructor, av_graphics_destructor);
}
