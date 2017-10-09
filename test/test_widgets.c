#include <avgl.h>

#define XRES 1024
#define YRES 768

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 200

static av_bool_t on_draw(av_visible_p self, av_graphics_p graphics)
{
	int r, g, b;
	long addr = (long)self;
	char buf[1024];
	int tw, th;
	printf("********** ON_DRAW!!!! \n");
	av_rect_t rect;
	av_rect_t absrect;
	av_window_p window = (av_window_p)self;
	window->get_rect(window, &rect);
	window->get_absolute_rect(window, &absrect);
	rect.x = rect.y = 0;
	graphics->rectangle(graphics, &rect);
	addr = (addr & 0xFFFF) ^ ((addr >> 16) & 0xFFFF);
	r=(unsigned char)(addr & 0xFF); addr>>=8;
	g=(unsigned char)(addr & 0xFF); addr>>=8;
	b=(unsigned char)(addr & 0xFF);
	graphics->set_color_rgba(graphics, (double)r/255, (double)g/255, (double)b/255, 0.8);
	//graphics->set_color_rgba(graphics, 0.5, 0, 1, 0.3);
	graphics->fill(graphics, 0);
	graphics->move_to(graphics, 0, 1.5);
	sprintf(buf, "%dx%d at %d,%d", absrect.w, absrect.h, absrect.x, absrect.y);
	graphics->get_text_extents(graphics, buf, &tw, &th, AV_NULL, AV_NULL, AV_NULL, AV_NULL);
	graphics->set_font_size(graphics, 1.5);
//	graphics->set_scale(graphics, 10, 10);
	graphics->text_path(graphics, buf);
	graphics->set_color_rgba(graphics, 1-(double)r/255, 1-(double)g/255, 1-(double)b/255, 1);
	graphics->stroke(graphics, 0);
//	graphics->set_scale(graphics, 1, 1);
	return AV_TRUE;
}

static av_bool_t dragging = AV_FALSE;
static av_bool_t is_position_change = AV_FALSE;
static int mouse_x = 0;
static int mouse_y = 0;

static av_bool_t on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	printf("%p button down\n", self);
	if (AV_MOUSE_BUTTON_LEFT == button )
	{
		mouse_x = x;
		mouse_y = y;
		avgl_capture_visible(self);
		self->raise_top(self);
		dragging = AV_TRUE;
		is_position_change = AV_TRUE;
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_RIGHT == button )
	{
		mouse_x = x;
		mouse_y = y;
		avgl_capture_visible(self);
		dragging = AV_TRUE;
		is_position_change = AV_FALSE;
		return AV_TRUE;
	}
	else if (AV_MOUSE_BUTTON_WHEEL == button)
	{
/*		self->origin_y ++;
		self->update(self, AV_UPDATE_INVALIDATE);*/
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	printf("%p button up\n", self);
	AV_UNUSED(x);
	AV_UNUSED(y);
	if ( (AV_MOUSE_BUTTON_LEFT == button) || (AV_MOUSE_BUTTON_RIGHT == button))
	{
		if (dragging)
		{
			avgl_capture_visible(AV_NULL);
			dragging = AV_FALSE;
			return AV_TRUE;
		}
	}
	else if (AV_MOUSE_BUTTON_WHEEL == button)
	{
/*		self->origin_y--;
		self->update(self, AV_UPDATE_INVALIDATE);*/
	}
	return AV_FALSE;
}

static av_bool_t on_mouse_enter(av_window_p self, int x, int y)
{
	printf("%p mouse enter %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_mouse_leave(av_window_p self, int x, int y)
{
	printf("%p mouse leave %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_mouse_hover(av_window_p self, int x, int y)
{
	printf("%p mouse hover %d %d\n", self, x, y);
	return AV_TRUE;
}

static av_bool_t on_mouse_move(av_window_p self, int x, int y)
{
	if (dragging)
	{
		av_rect_t rect;
		self->get_rect(self, &rect);
		if (is_position_change)
		{
			rect.x += (x - mouse_x);
			rect.y += (y - mouse_y);
		}
		else
		{
			rect.w += (x - mouse_x);
			rect.h += (y - mouse_y);
		}
		self->set_rect(self, &rect);
//		printf("%p set_rect %d %d %d %d\n", self, rect.x, rect.y, rect.w, rect.h);
		mouse_x = x;
		mouse_y = y;
//		((av_visible_p)self)->draw(self);
		return AV_TRUE;
	}
	return AV_FALSE;
}

av_window_p new_window(av_window_p parent, int x, int y, int w, int h)
{
	av_window_p window;
	window = (av_window_p)avgl_create_visible(parent, x, y, w, h, on_draw);
	window->set_hover_delay(window, 0);
	window->on_mouse_move        = on_mouse_move;
	window->on_mouse_button_down = on_mouse_button_down;
	window->on_mouse_button_up   = on_mouse_button_up;
	window->on_mouse_enter       = on_mouse_enter;
	window->on_mouse_hover       = on_mouse_hover;
	return window;
}

int test_widgets()
{
	av_window_p window;
	int i;
	av_display_config_t dc;
	dc.width = 20;
	dc.height = 18;
	dc.scale_x = 50;
	dc.scale_y = 40;
	dc.mode = 0;

	avgl_create(&dc);
//	avgl_create(AV_NULL);
	for (i=0; i<2; i++)
	{
		window = new_window(AV_NULL, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT);
		window->set_clip_children(window, 1);
		window = new_window(window, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT, AV_DEFAULT);
	}

	avgl_loop();
	avgl_destroy();
	return 1;
}
