/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics_cairo.c                                */
/* Description:   Cairo graphics implementation                      */
/*                                                                   */
/*********************************************************************/

#ifdef WITH_GRAPHICS_CAIRO
/* CAIRO graphics backend implementation */

#include <malloc.h>
#include <math.h>
#include <av_display.h>
#include <av_graphics.h>
#include <av_log.h>
#include <cairo.h>
#include <cairo-features.h>
#include "av_graphics_surface_cairo.h"

#define MAKE_VERSION_NUM(x,y,z) (z + (y << 8) + (x << 16))
#define CAIRO_VERSION_NUM MAKE_VERSION_NUM(CAIRO_VERSION_MAJOR, CAIRO_VERSION_MINOR, CAIRO_VERSION_MICRO)
#define CAIRO_NEWER(x,y,z) (MAKE_VERSION_NUM(x,y,z) <= CAIRO_VERSION_NUM)
#define CAIRO_OLDER(x,y,z) (MAKE_VERSION_NUM(x,y,z) > CAIRO_VERSION_NUM)

#define CONTEXT "graphics_cairo_ctx"
#define O_context(o) O_attr(o, CONTEXT)
#define O_context_surface(o) O_attr(o, CONTEXT_GRAPHICS_SURFACE)
#define O_context_pattern(o) O_attr(o, CONTEXT_GRAPHICS_PATTERN)

#define PITAGOR(x, y) sqrt((x)*(x) + (y)*(y))
#define SCALE_X(g, x) (x)*(((av_graphics_p)(g))->scale_x)
#define SCALE_Y(g, y) (y)*(((av_graphics_p)(g))->scale_y)
#define SCALE_R(g, r) (r)*(PITAGOR(((av_graphics_p)(g))->scale_x, ((av_graphics_p)(g))->scale_y))
#define UNSCALE_X(g, x) (x)/(((av_graphics_p)(g))->scale_x)
#define UNSCALE_Y(g, y) (y)/(((av_graphics_p)(g))->scale_y)
#define UNSCALE_R(g, r) (r)/(PITAGOR(((av_graphics_p)(g))->scale_x, ((av_graphics_p)(g))->scale_y))
/*
#define SCALE_X(g, x) (x)
#define SCALE_Y(g, y) (y)
#define SCALE_R(g, r) (r)
*/

/* exported registration method */
AV_API av_result_t av_graphics_cairo_register_oop(av_oop_p);

static av_log_p _log = AV_NULL;

/* Error messages */
static const char* _err_mem              = "out of memory";
static const char* _err_arg              = "invalid argument";
static const char* _err_found            = "not found error";
static const char* _err_restore          = "restore without matching save";
static const char* _err_current_point    = "no current point defined";
static const char* _err_matrix           = "invalid matrix (not invertible)";
static const char* _err_read             = "error while reading from input stream";
static const char* _err_write            = "error while writing to output stream";
static const char* _err_surface_finished = "target surface has been finished";
static const char* _err_surface          = "the surface type is not appropriate for the operation";
static const char* _err_pattern          = "the pattern type is not appropriate for the operation";

static av_result_t av_cairo_error_process(int rc, const char* funcname, const char* srcfilename, int linenumber)
{
	av_result_t averr = AV_OK;
	if (rc != CAIRO_STATUS_SUCCESS)
	{
		const char* errmsg = 0;
		switch (rc)
		{
			case CAIRO_STATUS_NO_MEMORY:        averr = AV_EMEM;              errmsg = _err_mem;              break;
			case CAIRO_STATUS_INVALID_RESTORE:  averr = AV_ERESTORE;          errmsg = _err_restore;          break;
			case CAIRO_STATUS_NO_CURRENT_POINT: averr = AV_ECURRENT_POINT;    errmsg = _err_current_point;    break;
			case CAIRO_STATUS_INVALID_MATRIX:   averr = AV_EMATRIX;           errmsg = _err_matrix;           break;
			case CAIRO_STATUS_NULL_POINTER:
			case CAIRO_STATUS_INVALID_STRING:
			case CAIRO_STATUS_INVALID_PATH_DATA:
			case CAIRO_STATUS_INVALID_CONTENT:
			case CAIRO_STATUS_INVALID_FORMAT:
			case CAIRO_STATUS_INVALID_DASH:
#if defined(CAIRO_STATUS_INVALID_DSC_COMMENT)
			case CAIRO_STATUS_INVALID_DSC_COMMENT:
#endif
			case CAIRO_STATUS_INVALID_VISUAL:   averr = AV_EARG;              errmsg = _err_arg;              break;
			case CAIRO_STATUS_READ_ERROR:       averr = AV_EREAD;             errmsg = _err_read;             break;
			case CAIRO_STATUS_WRITE_ERROR:      averr = AV_EWRITE;            errmsg = _err_write;            break;
			case CAIRO_STATUS_SURFACE_FINISHED: averr = AV_ESURFACE_FINISHED; errmsg = _err_surface_finished; break;
			case CAIRO_STATUS_SURFACE_TYPE_MISMATCH: averr = AV_ESURFACE;     errmsg = _err_surface;          break;
			case CAIRO_STATUS_PATTERN_TYPE_MISMATCH: averr = AV_EPATTERN;     errmsg = _err_pattern;          break;
			case CAIRO_STATUS_FILE_NOT_FOUND:   averr = AV_EFOUND;            errmsg = _err_found;            break;
			case CAIRO_STATUS_INVALID_POP_GROUP:av_assert(0, "error CAIRO_STATUS_INVALID_POP_GROUP");         break;
			case CAIRO_STATUS_INVALID_STATUS:   av_assert(0, "error CAIRO_STATUS_INVALID_STATUS");            break;
			default:
				if (_log) _log->error(_log, "%s returned error (%d) `unhandled error code' %s:%d",
										funcname, rc, srcfilename, linenumber);
				av_assert(0, "unhandled error");
			break;
		}
		if (errmsg)
		{
			if (_log) _log->error(_log, "%s returned error (%d) `%s' %s:%d",
									funcname, rc, errmsg, srcfilename, linenumber);
		}
		else
		{
			if (_log) _log->error(_log, "%s returned unknown error code (%d, `%s') %s:%d",
									funcname, rc, cairo_status_to_string(rc), srcfilename, linenumber);
		}
	}
	return averr;
}

#define av_cairo_error_check(funcname, rc) av_cairo_error_process(rc, funcname, __FILE__, __LINE__)

/* Create cairo graphics surface */
static av_result_t av_graphics_cairo_create_surface(av_graphics_p self,
													int width, int height,
													av_graphics_surface_p* ppsurface)
{
	av_result_t rc;
	av_graphics_surface_p surface;
	av_oop_p oop = O_oop(self);
	if (AV_OK != (rc = oop->new(oop, "graphics_surface_cairo", (av_object_p*)&surface)))
	{
		return rc;
	}

	/* set_size could expect a reference to the graphics owner object */
	surface->graphics = self;
	if (AV_OK != (rc = ((av_surface_p)surface)->set_size((av_surface_p)surface, width, height)))
	{
		O_destroy(surface);
		return rc;
	}
	*ppsurface = surface;
	return AV_OK;
}


static av_result_t av_graphics_cairo_create_surface_from_data(av_graphics_p self, int width, int height, av_pixel_p pixels, int pitch, av_graphics_surface_p* ppsurface)
{
	av_result_t rc;
	av_graphics_surface_p surface;
	av_oop_p oop = O_oop(self);
	cairo_surface_t *image = cairo_image_surface_create_for_data((unsigned char*)pixels, CAIRO_FORMAT_ARGB32, width, height, pitch);
	if (!image)
		return AV_EMEM;

	if (AV_OK != (rc = av_cairo_error_check("cairo_image_surface_create_for_data", cairo_surface_status(image))))
	{
		cairo_surface_destroy(image);
		return rc;
	}

	if (AV_OK != (rc = oop->new(oop, "graphics_surface_cairo", (av_object_p*)&surface)))
	{
		cairo_surface_destroy(image);
		return rc;
	}

	O_set_attr(surface, CONTEXT_GRAPHICS_SURFACE, image);
	surface->graphics = self;
	*ppsurface = surface;
	return AV_OK;
}

/* Create cairo graphics surface from file */
static av_result_t av_graphics_cairo_create_surface_from_file(av_graphics_p self, const char* filename, av_graphics_surface_p* ppsurface)
{
	av_result_t rc;
	av_graphics_surface_p surface;
	cairo_surface_t* image;
	av_oop_p oop = O_oop(self);

	/* FIXME: Detect file format */
	image = cairo_image_surface_create_from_png(filename);
	if (!image)
		return AV_EFOUND;

	rc = av_cairo_error_check("cairo_image_surface_create_from_png", cairo_surface_status(image));
	if (AV_OK != rc || AV_OK != (rc = oop->new(oop, "graphics_surface_cairo", (av_object_p*)&surface)))
	{
		cairo_surface_destroy(image);
		return rc;
	}

	O_set_attr(surface, CONTEXT_GRAPHICS_SURFACE, image);

	surface->graphics = self;
	*ppsurface = surface;
	return AV_OK;
}

/* Writes surface to file */
static av_result_t av_graphics_cairo_save_surface_file(av_graphics_p self,
													   av_graphics_surface_p surface,
													   const char* filename)
{
	cairo_surface_t* image = (cairo_surface_t*)O_context_surface(surface);
	AV_UNUSED(self);

	/* FIXME: Detect file format */
	return av_cairo_error_check("cairo_surface_write_to_png", cairo_surface_write_to_png(image, filename));
}

/*
*	Setup drawing environment on a target surface
*/
static av_result_t av_graphics_cairo_begin(av_graphics_p self, av_graphics_surface_p surface)
{
	cairo_t* cairo;
	cairo_surface_t* cairo_surface;
	av_assert(surface, "surface argument is missing");
	if (!(O_is_a(surface, "graphics_surface_cairo")))
		return AV_EARG;
	cairo_surface = O_context_surface(surface);
	av_assert(cairo_surface, "cairo surface is not initialized properly");

	cairo = cairo_create(cairo_surface);
	if (!cairo)
		return AV_EMEM;

	O_set_attr(self, CONTEXT, cairo);

	self->graphics_surface = surface;
	return AV_OK;
}

static av_result_t av_graphics_cairo_get_target_surface(av_graphics_p self,
														av_graphics_surface_p* ppgraphicssurface)
{
	*ppgraphicssurface = self->graphics_surface;
	return AV_OK;
}

/*
* Flushes collected drawing data on the target surface given with the method \c begin
*/
static void av_graphics_cairo_end(av_graphics_p self)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "calling `end' method before `begin'");
	cairo_destroy(cairo);
	O_set_attr(self, CONTEXT, 0);
	self->graphics_surface = AV_NULL;
}

/* Patterns implementation */

static av_result_t av_graphics_cairo_push_group(av_graphics_p self,
												av_graphics_content_t content)
{
	cairo_t* cairo = O_context(self);
	cairo_content_t cairo_content;
	av_assert(cairo, "calling `push_group' method before `begin'");
	switch (content)
	{
		case AV_CONTENT_COLOR: cairo_content = CAIRO_CONTENT_COLOR; break;
		case AV_CONTENT_ALPHA: cairo_content = CAIRO_CONTENT_ALPHA; break;
		case AV_CONTENT_COLOR_ALPHA: cairo_content = CAIRO_CONTENT_COLOR_ALPHA; break;
		default: return AV_EARG;
	}

	cairo_push_group_with_content(cairo, cairo_content);
	return av_cairo_error_check("cairo_push_group_with_content", cairo_status(cairo));
}

static av_result_t av_graphics_cairo_pop_group(av_graphics_p self,
											   av_graphics_pattern_p* ppattern)
{
	av_result_t rc;
	cairo_t* cairo = O_context(self);
	cairo_pattern_t* p;
	av_graphics_pattern_p pattern;
	av_oop_p oop = O_oop(self);
	av_assert(cairo, "calling `pop_group' method before `begin'");

	if (AV_OK != (rc = oop->new(oop, "graphics_pattern_cairo", (av_object_p*)&pattern)))
		return rc;

	p = cairo_pop_group(cairo);
	O_set_attr(pattern, CONTEXT_GRAPHICS_PATTERN, p);

	if (AV_OK != (rc = av_cairo_error_check("cairo_pop_group", cairo_pattern_status(p))))
	{
		O_destroy(pattern);
		pattern = AV_NULL;
	}
	*ppattern = pattern;
	return rc;
}

/*
* Creates RGBA pattern
*/
static av_result_t av_graphics_cairo_create_pattern_rgba(av_graphics_p self,
														 double r, double g, double b, double a,
														 av_graphics_pattern_p* ppattern)
{
	av_result_t rc;
	av_graphics_pattern_p pattern;
	cairo_pattern_t* p;
	av_oop_p oop = O_oop(self);
	p = cairo_pattern_create_rgba(r, g, b, a);
	if (AV_OK != (rc = oop->new(oop, "graphics_pattern_cairo", (av_object_p*)&pattern)))
	{
		cairo_pattern_destroy(p);
		return rc;
	}
	O_set_attr(pattern, CONTEXT_GRAPHICS_PATTERN, p);

	*ppattern = pattern;
	return AV_OK;
}

/*
* Creates linear pattern (for gradients)
*/
static av_result_t av_graphics_cairo_create_pattern_linear
		(av_graphics_p self, double xx0, double yy0, double xx1, double yy1,
		 av_graphics_pattern_p* ppattern)
{
	av_result_t rc;
	av_graphics_pattern_p pattern;
	cairo_pattern_t* p;
	av_oop_p oop = O_oop(self);
	p = cairo_pattern_create_linear(xx0, yy0, xx1, yy1);
	if (AV_OK != (rc = oop->new(oop, "graphics_pattern_cairo", (av_object_p*)&pattern)))
	{
		cairo_pattern_destroy(p);
		return rc;
	}
	O_set_attr(pattern, CONTEXT_GRAPHICS_PATTERN, p);

	*ppattern = pattern;
	return AV_OK;
}

/*
* Creates radial pattern (for gradients)
*/
static av_result_t av_graphics_cairo_create_pattern_radial
		(av_graphics_p self,
		 double cxx0, double cyy0, double r0, double cxx1, double cyy1, double r1,
		 av_graphics_pattern_p* ppattern)
{
	av_result_t rc;
	av_graphics_pattern_p pattern;
	cairo_pattern_t* p = cairo_pattern_create_radial(cxx0, cyy0, r0, cxx1, cyy1, r1);
	av_oop_p oop = O_oop(self);
	AV_UNUSED(self);
	if (!p)
		return AV_EMEM;

	if (AV_OK != (rc = oop->new(oop, "graphics_pattern_cairo", (av_object_p*)&pattern)))
	{
		cairo_pattern_destroy(p);
		return rc;
	}
	O_set_attr(pattern, CONTEXT_GRAPHICS_PATTERN, p);

	*ppattern = pattern;
	return AV_OK;
}

/*
* Set pattern
*/
static void av_graphics_cairo_set_pattern(av_graphics_p self,
										  av_graphics_pattern_p pattern)
{
	cairo_t* cairo = O_context(self);
	cairo_pattern_t* p = (cairo_pattern_t *)O_context_pattern(pattern);
	cairo_set_source(cairo, p);
}

/*
* Pattern add stop RGBA
*/
static av_result_t av_graphics_pattern_cairo_add_stop_rgba
			(av_graphics_pattern_p self, double offset, double r, double g, double b, double a)
{
	cairo_pattern_t* pattern = O_context_pattern(self);
	cairo_pattern_add_color_stop_rgba(pattern, offset, r, g, b, a);
	return av_cairo_error_check("cairo_pattern_add_color_stop_rgba", cairo_pattern_status(pattern));
}

static av_result_t av_graphics_pattern_cairo_set_extend(av_graphics_pattern_p self,
													    av_graphics_extend_t extend)
{
	cairo_extend_t cairo_extend;
	cairo_pattern_t* pattern = O_context_pattern(self);
	switch (extend)
	{
    	case AV_EXTEND_NONE: cairo_extend = CAIRO_EXTEND_NONE; break;
    	case AV_EXTEND_REPEAT: cairo_extend = CAIRO_EXTEND_REPEAT; break;
    	case AV_EXTEND_REFLECT: cairo_extend = CAIRO_EXTEND_REFLECT; break;
    	case AV_EXTEND_PAD: cairo_extend = CAIRO_EXTEND_PAD; break;
		default: return AV_EARG;
	}
	cairo_pattern_set_extend(pattern, cairo_extend);
	return AV_OK;
}

static av_result_t av_graphics_pattern_cairo_set_filter(av_graphics_pattern_p self,
													    av_filter_t filter)
{
	cairo_filter_t cairo_filter;
	cairo_pattern_t* pattern = O_context_pattern(self);
	switch (filter)
	{
    	case AV_FILTER_FAST: cairo_filter = CAIRO_FILTER_FAST; break;
    	case AV_FILTER_GOOD: cairo_filter = CAIRO_FILTER_GOOD; break;
    	case AV_FILTER_BEST: cairo_filter = CAIRO_FILTER_BEST; break;
    	case AV_FILTER_NEAREST: cairo_filter = CAIRO_FILTER_NEAREST; break;
    	case AV_FILTER_BILINEAR: cairo_filter = CAIRO_FILTER_BILINEAR; break;
    	case AV_FILTER_GAUSSIAN: cairo_filter = CAIRO_FILTER_GAUSSIAN; break;
		default: return AV_EARG;
	}
	cairo_pattern_set_filter(pattern, cairo_filter);
	return AV_OK;
}

/*
* Pattern constructor
*/
static av_result_t av_graphics_pattern_cairo_constructor(av_object_p pobject)
{
	av_graphics_pattern_p self = (av_graphics_pattern_p)pobject;
	self->add_stop_rgba = av_graphics_pattern_cairo_add_stop_rgba;
	self->set_extend    = av_graphics_pattern_cairo_set_extend;
	self->set_filter    = av_graphics_pattern_cairo_set_filter;
	return AV_OK;
}

/*
* Pattern destructor
*/
static void av_graphics_pattern_cairo_destructor(void* pobject)
{
	av_graphics_pattern_p self = (av_graphics_pattern_p)pobject;
	cairo_pattern_t *p = O_context_pattern(self);
	cairo_pattern_destroy(p);
}

/* Set clipping rectangle */
static av_result_t av_graphics_cairo_set_clip(av_graphics_p self, av_rect_p rect)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_reset_clip(cairo);
	self->set_line_width(self, 0);
	/*self->rectangle(self, rect);*/
	cairo_rectangle(cairo, rect->x, rect->y, rect->w, rect->h);
	cairo_clip(cairo);
	return av_cairo_error_check("cairo_clip", cairo_status(cairo));
}

/* Get clipping rectangle */
static av_result_t av_graphics_cairo_get_clip(av_graphics_p self, av_rect_p rect)
{
#if CAIRO_NEWER(1,5,0)
	double xx1, yy1, x2, y2;
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	cairo_clip_extents(cairo, &xx1, &yy1, &x2, &y2);
	rect->x = (int)xx1;
	rect->y = (int)yy1;
	rect->w = (int)x2 - rect->x;
	rect->h = (int)y2 - rect->y;
	return av_cairo_error_check("cairo_clip_extents", cairo_status(cairo));
#else
	AV_UNUSED(self);
	AV_UNUSED(rect);
	return AV_ESUPPORTED;
#endif
}

/*
* Saves the current graphics state in a stack which can be restored later via \c restore
*/
static av_result_t av_graphics_cairo_save(av_graphics_p self)
{
	cairo_t* cairo = O_context(self);

	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_save(cairo);
	return av_cairo_error_check("cairo_save", cairo_status(cairo));
}

/*
* Restores the last saved state via \c save from a stack and makes it active for later drawing methods.
*/
static av_result_t av_graphics_cairo_restore(av_graphics_p self)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_restore(cairo);
	return av_cairo_error_check("cairo_restore", cairo_status(cairo));
}

/*
*	Sets new position (x, y) of the current point
*/
static void av_graphics_cairo_move_to(av_graphics_p self, double x, double y)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_move_to(cairo, SCALE_X(self, x), SCALE_Y(self, y));
}

/*
*	Begin a new sub-path. After this call the current point will offset
*	by (\c dx, \c dy).
*
*	Given a current point of (x, y), rel_move_to(dx, dy)
*	is logically equivalent to move_to (x + dx, y + dy).
*/
static void av_graphics_cairo_rel_move_to(av_graphics_p self, double dx, double dy)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_rel_move_to(cairo, SCALE_X(self, dx), SCALE_Y(self, dy));
}

/*
*	Adds a line to the path from the current point to position (x, y)
*	in user-space coordinates. After this call the current point
*	will be (x, y)
*/
static av_result_t av_graphics_cairo_line_to(av_graphics_p self, double x, double y)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_line_to(cairo, SCALE_X(self, x), SCALE_Y(self, y));
	return av_cairo_error_check("cairo_line_to", cairo_status(cairo));
}

/*
*	Relative-coordinate version of \c line_to. Adds a line to the
*	path from the current point to a point that is offset from the
*	current point by (\c dx, \c dy) in user space. After this call the
*	current point will be offset by (\c dx, \c dy).
*/
static av_result_t av_graphics_cairo_rel_line_to(av_graphics_p self, double dx, double dy)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_rel_line_to(cairo, SCALE_X(self, dx), SCALE_Y(self, dy));
	return av_cairo_error_check("cairo_rel_line_to", cairo_status(cairo));
}

/*
*	Adds a cubic Bézier spline to the path from the current point to
*	position (\c x3, \c y3) in user-space coordinates, using (\c xx1, \c yy1) and
*	(\c x2, \c y2) as the control points. After this call the current point
*	will be (\c x3, \c y3).
*/
static av_result_t av_graphics_cairo_curve_to(av_graphics_p self,
											  double xx1_, double yy1_,
											  double x2_, double y2_,
											  double x3_, double y3_)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_curve_to(cairo, SCALE_X(self, xx1_),
						  SCALE_Y(self, yy1_),
						  SCALE_X(self, x2_),
						  SCALE_Y(self, y2_),
						  SCALE_X(self, x3_),
						  SCALE_Y(self, y3_));
	return av_cairo_error_check("cairo_curve_to", cairo_status(cairo));
}

/*
*	Relative-coordinate version of \c curve_to. All offsets are
*	relative to the current point. Adds a cubic Bézier spline to the
*	path from the current point to a point offset from the current
*	point by (\c dx3, \c dy3), using points offset by (\c dxx1, \c dyy1) and
*	(\c dx2, \c dy2) as the control points. After this call the current
*	point will be offset by (\c dx3, \c dy3).
*/
static av_result_t av_graphics_cairo_rel_curve_to(av_graphics_p self,
												  double dxx1, double dyy1,
												  double dx2, double dy2,
												  double dx3, double dy3)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_rel_curve_to(cairo, SCALE_X(self, dxx1),
							  SCALE_Y(self, dyy1),
							  SCALE_X(self, dx2),
							  SCALE_Y(self, dy2),
							  SCALE_X(self, dx3),
							  SCALE_Y(self, dy3));
	return av_cairo_error_check("cairo_curve_to", cairo_status(cairo));
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
static av_result_t av_graphics_cairo_arc(av_graphics_p self,
										 double cx,
										 double cy,
										 double radius,
										 double start_angle,
										 double end_angle)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	if (self->scale_x > self->scale_y)
		radius = SCALE_Y(self, radius);
	else
		radius = SCALE_X(self, radius);

	cairo_arc(cairo, SCALE_X(self, cx), SCALE_Y(self, cy), radius,
			  start_angle, end_angle);
	return av_cairo_error_check("cairo_arc", cairo_status(cairo));
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
static av_result_t av_graphics_cairo_arc_negative(av_graphics_p self,
												 double cx,
												 double cy,
												 double radius,
												 double start_angle,
												 double end_angle)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_arc_negative(cairo, SCALE_X(self, cx), SCALE_Y(self, cy), SCALE_R(self, radius),
					   start_angle, end_angle);
	return av_cairo_error_check("cairo_arc_negative", cairo_status(cairo));
}

/*
*	Adds a line segment to the path from the current point to the
*	beginning of the current sub-path
*/
static av_result_t av_graphics_cairo_close_path(av_graphics_p self)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_close_path(cairo);
	return av_cairo_error_check("cairo_close_path", cairo_status(cairo));
}

/*
*	Adds a closed sub-path rectangle of the given size to the current
*	path at position (x, y)
*/
static av_result_t av_graphics_cairo_rectangle(av_graphics_p self, av_rect_p rect)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_rectangle(cairo, SCALE_X(self, rect->x), SCALE_Y(self, rect->y),
						   SCALE_X(self, rect->w), SCALE_Y(self, rect->h));
	return av_cairo_error_check("cairo_rectangle", cairo_status(cairo));
}

/* Sets line width */
static av_result_t av_graphics_cairo_set_line_width(av_graphics_p self, double line_width)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_set_line_width(cairo, line_width);
	return av_cairo_error_check("cairo_set_line_width", cairo_status(cairo));
}

/* Sets line cap */
static av_result_t av_graphics_cairo_set_line_cap(av_graphics_p self, av_line_cap_t cap)
{
	cairo_t* cairo = O_context(self);
	cairo_line_cap_t cairo_cap = CAIRO_LINE_CAP_ROUND;
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	switch (cap)
	{
		case AV_LINE_CAP_BUTT:
			cairo_cap = CAIRO_LINE_CAP_BUTT;
		break;
		case AV_LINE_CAP_ROUND:
			cairo_cap = CAIRO_LINE_CAP_ROUND;
		break;
		case AV_LINE_CAP_SQUARE:
			cairo_cap = CAIRO_LINE_CAP_SQUARE;
		break;
		default: return AV_EARG;
	}

	cairo_set_line_cap(cairo, cairo_cap);
	return av_cairo_error_check("cairo_set_line_cap", cairo_status(cairo));
}

/* Sets line join */
static av_result_t av_graphics_cairo_set_line_join(av_graphics_p self, av_line_join_t join)
{
	cairo_t* cairo = O_context(self);
	cairo_line_join_t cairo_join = CAIRO_LINE_JOIN_ROUND;
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	switch (join)
	{
		case AV_LINE_JOIN_MITER:
			cairo_join = CAIRO_LINE_JOIN_MITER;
		break;
		case AV_LINE_JOIN_ROUND:
			cairo_join = CAIRO_LINE_JOIN_ROUND;
		break;
		case AV_LINE_JOIN_BEVEL:
			cairo_join = CAIRO_LINE_JOIN_BEVEL;
		break;
		default: return AV_EARG;
	}

	cairo_set_line_join(cairo, cairo_join);
	return av_cairo_error_check("cairo_set_line_join", cairo_status(cairo));
}

/* Draw contours around the painted objects */
static av_result_t av_graphics_cairo_stroke(av_graphics_p self, av_bool_t preserve)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	if (preserve)
		cairo_stroke_preserve(cairo);
	else
		cairo_stroke(cairo);
	return av_cairo_error_check("cairo_stroke", cairo_status(cairo));
}

/* A drawing operator that fills the current path according to the current fill rule */
static av_result_t av_graphics_cairo_fill(av_graphics_p self, av_bool_t preserve)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	if (preserve)
		cairo_fill_preserve(cairo);
	else
		cairo_fill(cairo);
	return av_cairo_error_check("cairo_fill", cairo_status(cairo));
}

/* A drawing operator that paints the current source everywhere within the current clip region. */
static av_result_t av_graphics_cairo_paint(av_graphics_p self, double alpha)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_paint_with_alpha(cairo, alpha);
	return av_cairo_error_check("cairo_paint_with_alpha", cairo_status(cairo));
}

static av_result_t av_graphics_cairo_flush(av_graphics_p self)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	 cairo_surface_flush(cairo_get_target(cairo));
	return av_cairo_error_check("cairo_surface_flush", cairo_status(cairo));
}

static void av_graphics_cairo_set_offset(av_graphics_p self, double dx, double dy)
{
	cairo_t* cairo = O_context(self);
	cairo_surface_t * target_surface;
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	target_surface = cairo_get_target(cairo);
	av_assert(target_surface, "target surface not defined for this graphics object");
	cairo_surface_set_device_offset(target_surface, dx, dy);
}

static void av_graphics_cairo_get_offset(av_graphics_p self, double* dx, double* dy)
{
	cairo_t* cairo = O_context(self);
	cairo_surface_t * target_surface;
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	target_surface = cairo_get_target(cairo);
	av_assert(target_surface, "target surface not defined for this graphics object");
	cairo_surface_get_device_offset(target_surface, dx, dy);
}

static void av_graphics_cairo_set_scale(av_graphics_p self, double sx, double sy)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	cairo_scale(cairo, sx, sy);
}

static void av_graphics_cairo_set_color_rgba(av_graphics_p self, double r, double g, double b, double a)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_set_source_rgba(cairo, r, g, b, a);
}

static void av_graphics_cairo_show_text(av_graphics_p self, const char* utf8)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_show_text(cairo, utf8);
}

static void av_graphics_cairo_text_path(av_graphics_p self, const char* utf8)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_text_path(cairo, utf8);
}

static void av_graphics_cairo_show_image(av_graphics_p self, double x, double y, av_graphics_surface_p surface)
{
	cairo_t* cairo = O_context(self);
	cairo_surface_t* cairo_surface = (cairo_surface_t*)O_context_surface(surface);
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	av_assert(cairo_surface, "image surface is not valid cairo surface");
	cairo_set_source_surface(cairo, cairo_surface, SCALE_X(self, x), SCALE_Y(self, y));
	cairo_paint(cairo);
}

static av_result_t av_graphics_cairo_get_text_extents(av_graphics_p self,
													  const char* utf8,
													  int* pwidth,
													  int* pheight,
													  int* pxbearing,
													  int* pybearing,
													  int* pxadvance,
													  int* pyadvance)
{
	int rc;
	cairo_t* cairo = O_context(self);
	cairo_text_extents_t text_extents;
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	cairo_text_extents(cairo, utf8, &text_extents);
	if (CAIRO_STATUS_SUCCESS == (rc = cairo_status(cairo)))
	{
		if (pwidth)
			*pwidth    = (int)floor(UNSCALE_X(self, text_extents.width));
		if (pheight)
			*pheight   = (int)floor(UNSCALE_Y(self, text_extents.height));
		if (pxbearing)
			*pxbearing = (int)floor(UNSCALE_X(self, text_extents.x_bearing));
		if (pybearing)
			*pybearing = (int)floor(UNSCALE_Y(self, text_extents.y_bearing));
		if (pxadvance)
			*pxadvance = (int)floor(UNSCALE_X(self, text_extents.x_advance));
		if (pyadvance)
			*pyadvance = (int)floor(UNSCALE_Y(self, text_extents.y_advance));
	}

	return av_cairo_error_check("cairo_text_extents", rc);
}

static av_result_t av_graphics_cairo_select_font_face(av_graphics_p self,
													  const char* fontface,
													  av_font_slant_t slant,
													  av_font_weight_t weight)
{
	cairo_t* cairo = O_context(self);
	cairo_font_slant_t cslant;
	cairo_font_weight_t cweight;
	av_assert(cairo, "method must be executed between `begin', `end' methods");

	switch (slant)
	{
		case AV_FONT_SLANT_NORMAL:
			cslant = CAIRO_FONT_SLANT_NORMAL;
		break;
		case AV_FONT_SLANT_ITALIC:
			cslant = CAIRO_FONT_SLANT_ITALIC;
		break;
		case AV_FONT_SLANT_OBLIQUE:
			cslant = CAIRO_FONT_SLANT_OBLIQUE;
		break;
		default:
			return AV_EARG;
	}

	switch (weight)
	{
		case AV_FONT_WEIGHT_NORMAL:
			cweight = CAIRO_FONT_WEIGHT_NORMAL;
		break;
		case AV_FONT_WEIGHT_BOLD:
			cweight = CAIRO_FONT_WEIGHT_BOLD;
		break;
		default:
			return AV_EARG;
	}

	cairo_select_font_face(cairo, fontface, cslant, cweight);
	return av_cairo_error_check("cairo_select_font_face", cairo_status(cairo));
}

static av_result_t av_graphics_cairo_set_font_size(av_graphics_p self, int size)
{
	cairo_t* cairo = O_context(self);
	av_assert(cairo, "method must be executed between `begin', `end' methods");
	cairo_set_font_size(cairo, AV_MIN(SCALE_X(self, size), SCALE_Y(self, size)));
	return av_cairo_error_check("cairo_set_font_size", cairo_status(cairo));
}

/*
* Graphics destructor
*/
static void av_graphics_cairo_destructor(void* pgraphics)
{
	AV_UNUSED(pgraphics);
}

/* Initializes memory given by the input pointer with the graphics cairo class information */
static av_result_t av_graphics_cairo_constructor(av_object_p object)
{
	av_graphics_p self = (av_graphics_p)object;

	self->graphics_surface    = AV_NULL;

	/* override abstract methods */
	self->create_surface      = av_graphics_cairo_create_surface;
	self->create_surface_from_data = av_graphics_cairo_create_surface_from_data;
	self->create_surface_from_file = av_graphics_cairo_create_surface_from_file;
	self->save_surface_file   = av_graphics_cairo_save_surface_file;
	self->begin               = av_graphics_cairo_begin;
	self->get_target_surface  = av_graphics_cairo_get_target_surface;
	self->end                 = av_graphics_cairo_end;
	self->push_group          = av_graphics_cairo_push_group;
	self->pop_group           = av_graphics_cairo_pop_group;
	self->create_pattern_rgba = av_graphics_cairo_create_pattern_rgba;
	self->create_pattern_linear = av_graphics_cairo_create_pattern_linear;
	self->create_pattern_radial = av_graphics_cairo_create_pattern_radial;
	self->set_pattern         = av_graphics_cairo_set_pattern;
	self->set_clip            = av_graphics_cairo_set_clip;
	self->get_clip            = av_graphics_cairo_get_clip;
	self->save                = av_graphics_cairo_save;
	self->restore             = av_graphics_cairo_restore;
	self->move_to             = av_graphics_cairo_move_to;
	self->rel_move_to         = av_graphics_cairo_rel_move_to;
	self->line_to             = av_graphics_cairo_line_to;
	self->rel_line_to         = av_graphics_cairo_rel_line_to;
	self->curve_to            = av_graphics_cairo_curve_to;
	self->rel_curve_to        = av_graphics_cairo_rel_curve_to;
	self->arc                 = av_graphics_cairo_arc;
	self->arc_negative        = av_graphics_cairo_arc_negative;
	self->close_path          = av_graphics_cairo_close_path;
	self->rectangle           = av_graphics_cairo_rectangle;
	self->set_line_width      = av_graphics_cairo_set_line_width;
	self->set_line_cap        = av_graphics_cairo_set_line_cap;
	self->set_line_join       = av_graphics_cairo_set_line_join;
	self->stroke              = av_graphics_cairo_stroke;
	self->fill                = av_graphics_cairo_fill;
	self->paint               = av_graphics_cairo_paint;
	self->flush               = av_graphics_cairo_flush;
	self->set_offset          = av_graphics_cairo_set_offset;
	self->get_offset          = av_graphics_cairo_get_offset;
	self->set_scale           = av_graphics_cairo_set_scale;
	self->set_color_rgba      = av_graphics_cairo_set_color_rgba;
	self->show_text           = av_graphics_cairo_show_text;
	self->text_path           = av_graphics_cairo_text_path;
	self->show_image          = av_graphics_cairo_show_image;
	self->get_text_extents    = av_graphics_cairo_get_text_extents;
	self->select_font_face    = av_graphics_cairo_select_font_face;
	self->set_font_size       = av_graphics_cairo_set_font_size;

	return AV_OK;
}

/* Registers Cairo graphics class into TORBA class repository */
AV_API av_result_t av_graphics_cairo_register_oop(av_oop_p oop)
{
	av_result_t rc;
	av_service_p graphics;

	if (AV_OK != (rc = av_graphics_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = av_graphics_surface_cairo_register_oop(oop)))
		return rc;

	if (AV_OK != (rc = oop->define_class(oop, "graphics_pattern_cairo",
											  AV_NULL, sizeof(av_graphics_pattern_t),
											  av_graphics_pattern_cairo_constructor,
											  av_graphics_pattern_cairo_destructor)))
		return rc;

	if (AV_OK != (rc = oop->define_class(oop, "graphics_cairo", "graphics", sizeof(av_graphics_t),
		av_graphics_cairo_constructor, av_graphics_cairo_destructor)))
		return rc;


	if (AV_OK != (rc = oop->new(oop, "graphics_cairo", (av_object_p*)&graphics)))
		return rc;

	/* register cairo graphics as service */
	return oop->register_service(oop, "graphics", graphics);
}

#endif /* WITH_GRAPHICS_CAIRO */
