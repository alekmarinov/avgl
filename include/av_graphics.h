/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_graphics.h                                      */
/*                                                                   */
/*********************************************************************/

/*! \file av_graphics.h
*   \brief av_graphics definition
*/

#ifndef __AV_GRAPHICS_H
#define __AV_GRAPHICS_H

#include <av.h>
#include <av_oop.h>
#include <av_surface.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief font slant
*/
typedef enum {
	AV_FONT_SLANT_NORMAL,
	AV_FONT_SLANT_ITALIC,
	AV_FONT_SLANT_OBLIQUE
} av_font_slant_t;

/*!
* \brief font weight
*/
typedef enum {
	AV_FONT_WEIGHT_NORMAL,
	AV_FONT_WEIGHT_BOLD
} av_font_weight_t;

/*!
* \brief font hint style
*/
typedef enum {
    AV_FONT_HINT_STYLE_DEFAULT,
    AV_FONT_HINT_STYLE_NONE,
    AV_FONT_HINT_STYLE_SLIGHT,
    AV_FONT_HINT_STYLE_MEDIUM,
    AV_FONT_HINT_STYLE_FULL
} av_font_hint_style_t;

/*!
* \brief font hint metrics
*/
typedef enum {
    AV_FONT_HINT_METRICS_DEFAULT,
    AV_FONT_HINT_METRICS_OFF,
    AV_FONT_HINT_METRICS_ON
} av_font_hint_metrics_t;

/*!
* \brief line caps
*/
typedef enum {
    AV_LINE_CAP_BUTT,
    AV_LINE_CAP_ROUND,
    AV_LINE_CAP_SQUARE
} av_line_cap_t;

/*!
* \brief line junctions
*/
typedef enum {
    AV_LINE_JOIN_MITER,
    AV_LINE_JOIN_ROUND,
    AV_LINE_JOIN_BEVEL
} av_line_join_t;

/*!
* \brief graphics extent types
*/
typedef enum {
    AV_EXTEND_NONE,
    AV_EXTEND_REPEAT,
    AV_EXTEND_REFLECT,
    AV_EXTEND_PAD
} av_graphics_extend_t;

/*!
* \brief graphics content types
*/
typedef enum {
    AV_CONTENT_COLOR,
    AV_CONTENT_ALPHA,
    AV_CONTENT_COLOR_ALPHA
} av_graphics_content_t;

/*!
* \brief filter types
*/
typedef enum {
    AV_FILTER_FAST,
    AV_FILTER_GOOD,
    AV_FILTER_BEST,
    AV_FILTER_NEAREST,
    AV_FILTER_BILINEAR,
    AV_FILTER_GAUSSIAN
} av_filter_t;

/* forward reference to av_graphics definition */
struct av_graphics;

/*!
* \brief pattern
*/
typedef struct av_graphics_pattern
{
	/*! parent class object */
	av_object_t object;

	/*!
	* \brief Adds stop RGBA color for gradient pattern
	* \param self is a reference to this object
	* \param offset is coordinate of the stop color (in pattern space)
	* \param r is the red color component
	* \param g is the green color component
	* \param b is the blue color component
	* \param a is the alpha color component
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*add_stop_rgba)(struct av_graphics_pattern* self,
								 double offset, double r, double g, double b, double a);

	/*!
	* \brief Sets pattern extend type
	* \param self is a reference to this object
	* \param extend any of av_graphics_extend_t
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_extend)(struct av_graphics_pattern* self,
							  av_graphics_extend_t extend);

	/*!
	* \brief Sets pattern filter type
	* \param self is a reference to this object
	* \param filter any of av_filter_t
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_filter)(struct av_graphics_pattern* self,
							  av_filter_t filter);

} av_graphics_pattern_t, *av_graphics_pattern_p;

/*!
* \brief graphics surface interface
*
* \c av_graphics_surface is the surface where
* the \c av_graphics rendering primitives will use to draw into
*/
typedef struct av_graphics_surface
{
	/*! Parent class surface */
	av_surface_t surface;

	/*! Graphics owner */
	struct av_graphics* graphics;

	/*! Creates pattern from this surface
	* \param self is a reference to this object
	* \param ppattern is the result pattern object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_pattern)(struct av_graphics_surface* self,
								  av_graphics_pattern_p* ppattern);

} av_graphics_surface_t, *av_graphics_surface_p;

/*!
* \brief graphics interface
*
*  Defines graphics rendering primitives
*/
typedef struct av_graphics
{
	/*! parent class object */
	av_service_t object;

	/*! target graphics surface */
	av_graphics_surface_p graphics_surface;

	/*! Scale factor X */
	double scale_x;

	/*! Scale factor Y */
	double scale_y;

	/*!
	* \brief Create graphics surface
	* \param self is a reference to this object
	* \param width of the surface
	* \param height of the surface
	* \param ppgraphicssurface result graphics surface object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*create_surface)(struct av_graphics* self,
								  int width,
								  int height,
								  av_graphics_surface_p* ppgraphicssurface);


	av_result_t (*create_surface_from_data)(struct av_graphics* self,
										    int width,
											int height,
											av_pixel_p pixels,
											int pitch,
											av_graphics_surface_p* ppsurface);

	/*!
	* \brief Create graphics surface from file
	* \param self is a reference to this object
	* \param filename of the image
	* \param ppgraphicssurface result graphics surface object
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND on file not found
	*         - AV_ESUPPORTED on not supported file format
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*create_surface_from_file)(struct av_graphics* self,
									        const char* filename,
									        av_graphics_surface_p* ppgraphicssurface);

	/*!
	* \brief Writes graphics surface to file
	* \param self is a reference to this object
	* \param graphicssurface result graphics surface object
	* \param filename is the name of image to create from the given surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*save_surface_file)(struct av_graphics* self,
									 av_graphics_surface_p graphicssurface,
									 const char* filename);

	/*!
	* \brief Setup drawing environment over a target graphics surface
	* \param self is a reference to this object
	* \param graphicssurface is the target surface where the graphics will paint into
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EARG if \c graphicssurface parameter is not derived from \c av_graphics_surface
	*         - AV_EMEM on out of memory
	*/
	av_result_t (*begin)         (struct av_graphics* self,
								  av_graphics_surface_p graphicssurface);

	/*!
	* \brief Get target surface used as a drawing environment
	* \param self is a reference to this object
	* \param ppgraphicssurface result graphics target surface
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_target_surface)(struct av_graphics* self,
									  av_graphics_surface_p* ppgraphicssurface);

	/*!
	* \brief Finish drawing started with method \c begin
	* \param self is a reference to this object
	*/
	void (*end) (struct av_graphics* self);

	/*!
	* \brief Pushes group to stack
	* Temporarily redirects drawing to an intermediate surface known as a
	* group. The redirection lasts until the group is completed by a call
	* to \c pop_group or \c pop_group_to_source. These calls
	* provide the result of any drawing to the group as a pattern
	* \param self is a reference to this object
	* \param content is any of av_graphics_content_t
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*push_group) (struct av_graphics* self, av_graphics_content_t content);

	/*!
	* \brief Pops group from stack
	* Terminates the redirection begun by a call to \c push_group
	* and returns a new pattern containing the results of all drawing
	* operations performed to the group.
	* \param self is a reference to this object
	* \param ppattern is the result pattern object created by the popped group
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*pop_group) (struct av_graphics* self,
							  av_graphics_pattern_p* ppattern);

	/*!
	* \brief Creates new pattern by RGBA color
	* \param self is a reference to this object
	* \param r is the red color component
	* \param g is the green color component
	* \param b is the blue color component
	* \param a is the alpha color component
	* \param ppattern the result pattern object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_pattern_rgba)(struct av_graphics* self,
									   double r, double g, double b, double a,
									   av_graphics_pattern_p* ppattern);

	/*!
	* \brief Creates new linear pattern (linear gradient)
	* \param self is a reference to this object
	* \param x0 coordinate of the start point
	* \param y0 coordinate of the start point
	* \param x1 coordinate of the end point
	* \param y1 coordinate of the end point
	* \param ppattern the result pattern object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_pattern_linear)(struct av_graphics* self,
									   double x0, double y0, double x1, double y1,
									   av_graphics_pattern_p* ppattern);

	/*!
	* \brief Creates new radial pattern (radial gradient)
	* \param self is a reference to this object
	* \param cx0 x coordinate for the center of the start circle
	* \param cy0 y coordinate for the center of the start circle
	* \param r0 radius of the start circle
	* \param cx1 x coordinate for the center of the end circle
	* \param cy1 y coordinate for the center of the end circle
	* \param r1 radius of the end circle
	* \param ppattern the result pattern object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*create_pattern_radial)(struct av_graphics* self,
									   double cx0, double cy0, double r0,
									   double cx1, double cy1, double r1,
									   av_graphics_pattern_p* ppattern);

	/*!
	* \brief Creates pattern
	* \param self is a reference to this object
	* \param pattern is a reference to pattern object
	*/
	void (*set_pattern)(struct av_graphics* self, av_graphics_pattern_p pattern);

	/*!
	* \brief Set clipping rectangle
	* \param self is a reference to this object
	* \param cliprect is the clipping rectangle
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_clip)      (struct av_graphics* self, av_rect_p cliprect);

	/*!
	* \brief Get clipping rectangle
	* \param self is a reference to this object
	* \param cliprect points the result clipping rectangle
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*get_clip)      (struct av_graphics* self, av_rect_p cliprect);

	/*!
	* \brief Saves the current graphics state in a stack which can be restored later via \c restore
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*save)          (struct av_graphics* self);

	/*!
	* \brief Restores the last saved state via \c save from a stack
	* and makes it active for later drawing methods
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*restore)       (struct av_graphics* self);

	/*!
	* \brief Sets new position (\c x, \c y) of the current point
	* \param self is a reference to this object
	* \param x move to position
	* \param y move to position
	* \sa rel_move_to, line_to, rel_line_to, curve_to, rel_curve_to
	*/
	void (*move_to)              (struct av_graphics* self, double x, double y);

	/*!
	* \brief Begin a new sub-path. After this call the current point will offset
	* by (\c dx, \c dy).
	*
	* Given a current point of (x, y), rel_move_to(dx, dy)
	* is logically equivalent to move_to (x + dx, y + dy)
	* \param self is a reference to this object
	* \param dx offset by X axis from the current point
	* \param dy offset by Y axis from the current point
	* \sa move_to, line_to, rel_line_to, curve_to, rel_curve_to
	*/
	void (*rel_move_to)          (struct av_graphics* self, double dx, double dy);

	/*!
	* \brief Adds a line to the path from the current point to position (\c x, \c y)
	* in user-space coordinates. After this call the current point
	* will be (\c x, \c y)
	* \param self is a reference to this object
	* \param x is the X axis for the second point of the line
	* \param y is the Y axis for the second point of the line
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa rel_move_to, move_to, rel_line_to, curve_to, rel_curve_to
	*/
	av_result_t (*line_to)       (struct av_graphics* self, double x, double y);

	/*!
	* \brief Relative-coordinate version of \c line_to. Adds a line to the
	* path from the current point to a point that is offset from the
	* current point by (\c dx, \c dy) in user space. After this call the
	* current point will be offset by (\c dx, \c dy)
	* \param self is a reference to this object
	* \param dx offset by X axis from the current point for second line point
	* \param dy offset by Y axis from the current point for second line point
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa rel_move_to, move_to, line_to, curve_to, rel_curve_to
	*/
	av_result_t (*rel_line_to)   (struct av_graphics* self, double dx, double dy);

	/*!
	* \brief Adds a cubic Bézier spline to the path from the current point to
	* position (\c x3, \c y3) in user-space coordinates, using (\c x1, \c y1) and
	* (\c x2, \c y2) as the control points. After this call the current point
	* will be (\c x3, \c y3).
	* \param self is a reference to this object
	* \param x1 is the X axis for the first control point
	* \param y1 is the Y axis for the first control point
	* \param x2 is the X axis for the second control point
	* \param y2 is the Y axis for the second control point
	* \param x3 is the X axis for the end curve point
	* \param y3 is the Y axis for the end curve point
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa rel_move_to, move_to, line_to, rel_line_to, rel_curve_to
	*/
	av_result_t (*curve_to)      (struct av_graphics* self,
								  double x1_, double y1_,
								  double x2_, double y2_,
								  double x3_, double y3_);

	/*!
	* \brief Relative-coordinate version of \c curve_to. All offsets are
	* relative to the current point. Adds a cubic Bézier spline to the
	* path from the current point to a point offset from the current
	* point by (\c dx3, \c dy3), using points offset by (\c dx1, \c dy1) and
	* (\c dx2, \c dy2) as the control points. After this call the current
	* point will be offset by (\c dx3, \c dy3).
	* \param self is a reference to this object
	* \param dx1 is offset by X axis from the current point for the first control point
	* \param dy1 is offset by Y axis from the current point for the first control point
	* \param dx2 is offset by X axis from the current point for the second control point
	* \param dy2 is offset by Y axis from the current point for the second control point
	* \param dx3 is offset by X axis from the current point for the end control point
	* \param dy3 is offset by Y axis from the current point for the end control point
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa rel_move_to, move_to, line_to, rel_line_to, curve_to
	*/
	av_result_t (*rel_curve_to)  (struct av_graphics* self,
								  double dx1, double dy1,
								  double dx2, double dy2,
								  double dx3, double dy3);

	/*!
	* \brief Adds a circular arc of the given \c radius to the current path.  The
	* arc is centered at (\c xc, \c yc), begins at \c start_angle and proceeds in
	* the direction of increasing angles to end at \c end_angle. If \c end_angle is
	* less than \c start_angle it will be progressively increased by 2*M_PI
	* until it is greater than \c start_angle
	*
	* If there is a current point, an initial line segment will be added
	* to the path to connect the current point to the beginning of the
	* arc.
	*
	* Angles are measured in radians. An angle of 0.0 is in the direction
	* of the positive X axis (in user space). An angle of M_PI/2.0 radians
	* (90 degrees) is in the direction of the positive Y axis (in
	* user space). Angles increase in the direction from the positive X
	* axis toward the positive Y axis. So with the default transformation
	* matrix, angles increase in a clockwise direction.
	*
	* This function gives the arc in the direction of increasing angles;
	* \see arc_negative to get the arc in the direction of decreasing angles.
	*
	* The arc is circular in user space. To achieve an elliptical arc,
	* you can scale the current transformation matrix by different
	* amounts in the X and Y directions.
	*
	* \param self is a reference to this object
	* \param xc coordinate to the arc circle center
	* \param yc coordinate to the arc circle center
	* \param radius of the arc circle
	* \param start_angle of the arc
	* \param end_angle of the arc
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa arc_negative
	*/
	av_result_t (*arc)           (struct av_graphics* self,
								  double xc,
								  double yc,
								  double radius,
								  double start_angle,
								  double end_angle);

	/*!
	* \brief Adds a circular arc of the given \c radius to the current path.  The
	* arc is centered at (\c xc, \c yc), begins at \c start_angle and proceeds in
	* the direction of decreasing angles to end at \c end_angle. If \c end_angle is
	* greater than \c start_angle it will be progressively decreased by 2*M_PI
	* until it is less than \c start_angle
	*
	* \c see arc for more details. This function differs only in the
	* direction of the arc between the two angles.
	*
	* \param self is a reference to this object
	* \param xc coordinate to the arc circle center
	* \param yc coordinate to the arc circle center
	* \param radius of the arc circle
	* \param start_angle of the arc
	* \param end_angle of the arc
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	* \sa arc
	*/
	av_result_t (*arc_negative)  (struct av_graphics* self,
								  double xc,
								  double yc,
								  double radius,
								  double start_angle,
								  double end_angle);

	/*!
	* \brief Adds a line segment to the path from the current point to the
	* beginning of the current sub-path
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*close_path)    (struct av_graphics* self);

	/*!
	* \brief Adds a closed sub-path rectangle
	* \param self is a reference to this object
	* \param rect is a rectangular area
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*rectangle)     (struct av_graphics* self, av_rect_p rect);

	/*!
	* \brief Sets line width
	* \param self is a reference to this object
	* \param width is the new line width
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_line_width)(struct av_graphics* self, double width);

	/*!
	* \brief Sets line cap
	* \param self is a reference to this object
	* \param cap specifies how to render the endpoint of a line when stroking
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_line_cap)(struct av_graphics* self, av_line_cap_t cap);

	/*!
	* \brief Sets line join
	* \param self is a reference to this object
	* \param join Specifies how to render the junction of two lines when stroking.
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*set_line_join)(struct av_graphics* self, av_line_join_t join);

	/*!
	* \brief Draw contours around the painted objects
	* \param self is a reference to this object
	* \param preserve is telling if the current path must be kept active
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*stroke)        (struct av_graphics* self, av_bool_t preserve);

	/*!
	* \brief A drawing operator that fills the current path according to the current fill rule
	* \param self is a reference to this object
	* \param preserve is telling if the current path must be kept active
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*fill)          (struct av_graphics* self, av_bool_t preserve);

	/*!
	* \brief A drawing operator that paints the current source everywhere within the current clip region.
	* \param self is a reference to this object
	* \param alpha is a translucent level used while painting
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*paint)         (struct av_graphics* self, double alpha);

	/*!
	* \brief Finishes all current drawings on the target surface
	* \param self is a reference to this object
	* \return av_result_t
	*         - AV_OK on success
	*         - != AV_OK on failure
	*/
	av_result_t (*flush)         (struct av_graphics* self);

	/*!
	* \brief Sets an offset that is added to the coordinates when drawing to the graphics surface
	* \param self is a reference to this object
	* \param dx offset by X axis
	* \param dy offset by Y axis
	*/
	void (*set_offset)  (struct av_graphics* self, double dx, double dy);

	/*!
	* \brief Gets an offset that is added to the coordinates when drawing to the graphics surface
	* \param self is a reference to this object
	* \param dx offset by X axis
	* \param dy offset by Y axis
	*/
	void (*get_offset)  (struct av_graphics* self, double* dx, double* dy);

	/*!
	* \brief Scales X,Y user-space axes
	* \param self is a reference to this object
	* \param sx is the scale factor for the X dimension
	* \param sy is the scale factor for the Y dimension
	*/
	void (*set_scale)   (struct av_graphics* self, double sx, double sy);

	/*!
	* Set RGBA color
	* \param self is a reference to this object
	* \param r is the red color component
	* \param g is the green color component
	* \param b is the blue color component
	* \param a is the alpha color component
	*/
	void (*set_color_rgba) (struct av_graphics* self, double r, double g, double b, double a);

	void (*text_path)      (struct av_graphics* self, const char* utf8);
	void (*show_text)      (struct av_graphics* self, const char* utf8);
	void (*show_image)     (struct av_graphics* self, double x, double y, av_graphics_surface_p image);

	av_result_t (*get_text_extents)   (struct av_graphics* self,
									   const char* utf8,
									   int* pwidth,
									   int* pheight,
									   int* pxbearing,
									   int* pybearing,
									   int* pxadvance,
									   int* pyadvance);

	av_result_t (*select_font_face)   (struct av_graphics* self, const char* fontface, av_font_slant_t slant, av_font_weight_t weight);
	av_result_t (*set_font_size)      (struct av_graphics* self, int size);

} av_graphics_t, *av_graphics_p;

/*!
* \brief Registers graphics class into OOP
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_graphics_register_oop(av_oop_p);

#ifdef __cplusplus
}
#endif

#endif /* __AV_GRAPHICS_H */
