/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_rect.c                                          */
/* Description:   rectangle operations                               */
/*                                                                   */
/*********************************************************************/

#include <av_rect.h>
#include <malloc.h>

av_bool_t av_rect_init(av_rect_p rect, int x, int y, int w, int h)
{
	av_bool_t res = AV_TRUE;
	av_assert(rect, "NULL rectangle argument is now allowed");

	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;

	if (w < 0)
	{
		rect->w = 0;
		res = AV_FALSE;
	}

	if (h < 0)
	{
		rect->h = 0;
		res = AV_FALSE;
	}
	return res;
}

av_bool_t av_rect_compare(av_rect_p rect1, av_rect_p rect2)
{
	return (rect1->x == rect2->x) && (rect1->y == rect2->y) && (rect1->w == rect2->w) && (rect1->h == rect2->h);
}

av_bool_t av_rect_point_inside(av_rect_p rect, int x, int y)
{
	av_assert(rect, "NULL rectangle argument is now allowed");
	return ((x >= rect->x) && x < (rect->x + rect->w) && 
			(y >= rect->y) && y < (rect->y + rect->h));
}

av_bool_t av_rect_contains(av_rect_p rect1, av_rect_p rect2)
{
	int right1, bottom1, right2, bottom2;
	av_assert(rect1 && rect2, "NULL rectangle argument is now allowed");

	right1  = rect1->x + rect1->w - 1;
	bottom1 = rect1->y + rect1->h - 1;
	right2  = rect2->x + rect2->w - 1;
	bottom2 = rect2->y + rect2->h - 1;
	
	return (rect2->x >= rect1->x && rect2->y >= rect1->y && right2 <= right1 && bottom2 <= bottom1);
}

av_bool_t av_rect_intersect(av_rect_p rect1, av_rect_p rect2, av_rect_p rect)
{
	int right1, bottom1, right2, bottom2;
	av_assert(rect1 && rect2 && rect, "NULL rectangle argument is now allowed");

	right1  = rect1->x + rect1->w - 1;
	bottom1 = rect1->y + rect1->h - 1;
	right2  = rect2->x + rect2->w - 1;
	bottom2 = rect2->y + rect2->h - 1;

	if ((right2 >= rect1->x)  && (rect2->x <= right1) &&
		(bottom2 >= rect1->y) && (rect2->y <= bottom1))
	{
		int x[4], y[4];
		int i, j, temp;
		av_bool_t valid;

		x[0] = rect1->x;
		x[1] = right1;
		x[2] = rect2->x;
		x[3] = right2;

		y[0] = rect1->y;
		y[1] = bottom1;
		y[2] = rect2->y;
		y[3] = bottom2;

		for(i=0; i<3; i++)
			for(j=0; j<3-i; j++)
			{
				if(x[j] > x[j+1])
				{
					temp = x[j];
					x[j] = x[j+1];
					x[j+1] = temp;
				}
				if(y[j]>y[j+1])
				{
					temp = y[j];
					y[j] = y[j+1];
					y[j+1] = temp;
				}
			}

		valid = av_rect_init(rect, x[1], y[1], x[2] - x[1] + 1, y[2] - y[1] + 1);
		av_assert(valid, "Error in intersection algorithm. The result width or height is not positive defined");
		return AV_TRUE;
	}
	return AV_FALSE;
}

av_result_t av_rect_substract(av_rect_p rectA, av_rect_p rectB, av_list_p* pplist)
{
	av_list_p rlist = AV_NULL;
	av_result_t rc = AV_EARG;
	av_rect_t irect;
	av_rect_p rect;

	av_assert(rectA && rectB, "NULL rectangles are not allowed");

	if (av_rect_intersect(rectA, rectB, &irect))
	{
		int i;
		if (AV_OK != (rc = av_list_create(&rlist)))
		{
			return rc;
		}

		for (i=0; i<4; i++)
		{
			int x, y, w, h;
			switch (i)
			{
				case 0: /* left rect */
					x = rectA->x;
					y = irect.y;
					w = irect.x - rectA->x;
					h = irect.h;
				break;

				case 1: /* right rect */
					x = irect.x + irect.w;
					y = irect.y;
					w = rectA->x + rectA->w - irect.x - irect.w;
					h = irect.h;
				break;

				case 2: /* top rect */
					x = rectA->x;
					y = rectA->y;
					w = rectA->w;
					h = irect.y - rectA->y;
				break;

				case 3: /* bottom rect */
					x = rectA->x;
					y = irect.y + irect.h;
					w = rectA->w;
					h = rectA->y + rectA->h - irect.h - irect.y;
				break;
			}

			if (w > 0 && h > 0)
			{
				rect = (av_rect_p)malloc(sizeof(av_rect_t));
				if (!rect)
				{
					rc = AV_EMEM;
					goto av_rect_substract_err;
				}
				av_rect_init(rect, x, y, w, h);
				if (AV_OK != (rc = rlist->push_last(rlist, rect)))
				{
					goto av_rect_substract_err;
				}
			}
		}
		*pplist = rlist;
		rc = AV_OK;
	}
	else
		rc = AV_EFOUND;

	return rc;

av_rect_substract_err:
	if (rlist)
	{
		rlist->iterate_all(rlist, free, AV_TRUE);
		rlist->destroy(rlist);
	}
	return rc;
}

void av_rect_move(av_rect_p rect, int ofs_x, int ofs_y)
{
	av_assert(rect, "NULL rectangle argument is now allowed");
	rect->x += ofs_x;
	rect->y += ofs_y;
}

av_bool_t av_rect_resize(av_rect_p rect, int offs_w, int offs_h)
{
	av_bool_t res = AV_TRUE;
	av_assert(rect, "NULL rectangle argument is now allowed");

	rect->w += offs_w;
	rect->h += offs_h;
	if (rect->w < 0)
	{
		rect->w = 0;
		res = AV_FALSE;
	}

	if (rect->h < 0)
	{
		rect->h = 0;
		res = AV_FALSE;
	}

	return res;
}

void av_rect_set_size(av_rect_p rect, int new_w, int new_h)
{
	av_assert(rect, "NULL rectangle argument is now allowed");
	av_assert(new_w >= 0, "Invalid rectangle width");
	av_assert(new_h >= 0, "Invalid rectangle height");

	rect->w = new_w;
	rect->h = new_h;
}

void av_rect_copy(av_rect_p rectout, av_rect_p rectin)
{
	av_assert(rectout && rectin, "NULL rectangle argument is now allowed");
	*rectout = *rectin;
}

void av_rect_extend(av_rect_p rect_extend, av_rect_p rect_cover)
{
	av_assert(rect_extend, "NULL rectangle argument is now allowed");
	if (rect_cover)
	{
		/* extend left */
		if (rect_cover->x < rect_extend->x)
		{
			rect_extend->w += rect_extend->x - rect_cover->x;
			rect_extend->x = rect_cover->x;

			/* extend right */
			if (rect_extend->w < rect_cover->w)
				rect_extend->w = rect_cover->w;
		}
		else
		{
			int x1 = rect_extend->x + rect_extend->w;
			int x2 = rect_cover->x + rect_cover->w;
			rect_extend->w = AV_MAX(x1, x2) - rect_extend->x;
		}

		/* extend top */
		if (rect_cover->y < rect_extend->y)
		{
			rect_extend->h += rect_extend->y - rect_cover->y;
			rect_extend->y = rect_cover->y;

			/* extend bottom */
			if (rect_extend->h < rect_cover->h)
				rect_extend->h = rect_cover->h;
		}
		else
		{
			int y1 = rect_extend->y + rect_extend->h;
			int y2 = rect_cover->y + rect_cover->h;
			rect_extend->h = AV_MAX(y1, y2) - rect_extend->y;
		}
	}
}
