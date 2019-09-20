/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2018,  Alexander Marinov                            */
/*                                                                   */
/* Project:       lavgl                                              */
/* Filename:      lavgl.c                                            */
/* Description:   Main binding interface                             */
/*                                                                   */
/*********************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include "lavgl.h"

static void new_lua_surface(lua_State* L, av_surface_p surface);
static void new_lua_visible(lua_State* L, av_visible_p visible);
static void new_lua_graphics(lua_State* L, av_graphics_p graphics);
static void new_lua_system(lua_State* L, av_system_p system);
static void new_lua_graphics_surface(lua_State* L, av_graphics_surface_p graphics_surface);
static void new_lua_graphics_pattern(lua_State* L, av_graphics_pattern_p graphics_pattern);

#define totype(L, T, i) *(T *)lua_touserdata(L, i)
#define toobject(L, i) totype(L, av_object_p, i)
#define tosurface(L, i) totype(L, av_surface_p, i)
#define tosprite(L, i) totype(L, av_sprite_p, i)
#define tosystem(L, i) totype(L, av_system_p, i)
#define towindow(L, i) totype(L, av_window_p, i)
#define tovisible(L, i) totype(L, av_visible_p, i)
#define tographics(L, i) totype(L, av_graphics_p, i)
#define tographics_surface(L, i) totype(L, av_graphics_surface_p, i)
#define tographics_pattern(L, i) totype(L, av_graphics_pattern_p, i)

#define check_result(L, rc) \
	if (rc != AV_OK) \
	{ \
		lua_pushnil(L); \
		lua_pushinteger(L, rc); \
		return 2; \
	}

static char *build_registry_name(av_object_p object)
{
	av_object_p* pobject = O_attr(object, "_userdata");
	char* rname = (char*)av_malloc(255);
	snprintf(rname, 255, "%s_%p", object->classref->classname, pobject);
	return rname;
}

static int* luatable_tointarray(lua_State* L, int index)
{
	int i, n;
	int* arr;
	lua_len(L, index);
	n = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	arr = (int*)malloc((1+n) * sizeof(int));
	arr[0] = n;
	for (i = 1; i <= n; i++)
	{
		int val;
		lua_pushinteger(L, i);
		lua_gettable(L, index);
		val = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		arr[i] = val;
	}
	return arr;
}

void avlua_rect_new(lua_State* L, av_rect_p rect)
{
	lua_newtable(L);
	lua_pushinteger(L, rect->x);
	lua_seti(L, -2, 1);
	lua_pushinteger(L, rect->y);
	lua_seti(L, -2, 2);
	lua_pushinteger(L, rect->w);
	lua_seti(L, -2, 3);
	lua_pushinteger(L, rect->h);
	lua_seti(L, -2, 4);
}

void avlua_torect(lua_State* L, int cindex, av_rect_p rect)
{
	if (lua_istable(L, cindex))
	{
		/* check if table is given as {1, 2, 3, 4} or {x=1,y=2,w=3,h=4} */
		lua_getfield(L, cindex, "x");
		if (lua_isnumber(L, -1))
		{
			/* expected table like {x=1,y=2,w=3,h=4} */
			rect->x = (int)luaL_checkinteger(L, -1);
			lua_getfield(L, cindex, "y");
			rect->y = (int)luaL_checkinteger(L, -1);
			lua_getfield(L, cindex, "w");
			rect->w = (int)luaL_checkinteger(L, -1);
			lua_getfield(L, cindex, "h");
			rect->h = (int)luaL_checkinteger(L, -1);
			lua_pop(L, 4);
		}
		else
		{
			lua_rawgeti(L, cindex, 1);
			rect->x = (int)luaL_checkinteger(L, -1);
			lua_rawgeti(L, cindex, 2);
			rect->y = (int)luaL_checkinteger(L, -1);
			lua_rawgeti(L, cindex, 3);
			rect->w = (int)luaL_checkinteger(L, -1);
			lua_rawgeti(L, cindex, 4);
			rect->h = (int)luaL_checkinteger(L, -1);
			lua_pop(L, 5);
		}
	}
	else
	{
		rect->x = (int)luaL_checkinteger(L, 0 + cindex);
		rect->y = (int)luaL_checkinteger(L, 1 + cindex);
		rect->w = (int)luaL_checkinteger(L, 2 + cindex);
		rect->h = (int)luaL_checkinteger(L, 3 + cindex);
	}
}

/* object */
static int lobject_gc(lua_State* L)
{
	av_object_p* pobject = (av_object_p *)lua_touserdata(L, 1);
	O_release(*pobject);
	*pobject = AV_NULL;
	return 0;
}

static int lobject_attr(lua_State* L)
{
	av_object_p object = toobject(L, 1);
	const char* key = luaL_checkstring(L, 2);
	lua_pushlightuserdata(L, O_attr(object, key));
	return 1;
}

static const struct luaL_Reg lobject_meths[] =
{
	{ "attr", lobject_attr },
	{ "__gc", lobject_gc },
	{ AV_NULL, AV_NULL }
};

static lua_State* avlua_push_object(av_object_p object)
{
	lua_State* L = (lua_State*)O_attr(object, "_L");
	char* rname = build_registry_name(object);
	if (luaL_getmetatable(L, rname) == LUA_TNIL) {
		lua_pushfstring(L, "Registry name %s is not defined", rname);
		av_free(rname);
		lua_error(L);
	}
	av_free(rname);
	return L;
}

static void new_lua_object(lua_State* L, av_object_p object)
{
	av_object_p* pobject = (av_object_p*)lua_newuserdata(L, sizeof(av_object_p*));
	char* rname;
	printf("new_lua_object %p as %s\n", object, object->classref->classname);
	O_set_attr(object, "_userdata", pobject);
	O_set_attr(object, "_L", L);
	*pobject = O_addref(object);
	rname = build_registry_name(object);
	if (luaL_newmetatable(L, rname) == 0)
	{
		lua_pushfstring(L, "Registry name %s is already in use", rname);
		av_free(rname);
		lua_error(L);
	}
	av_free(rname);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__newindex");  /* metatable.__newindex = metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
	lua_setmetatable(L, -2);
	luaL_setfuncs(L, lobject_meths, 0);
}

/* surface */

static int lsurface_setsize(lua_State* L)
{
	av_surface_p surface = tosurface(L, 1);
	int width = (int)luaL_checkinteger(L, 2);
	int height = (int)luaL_checkinteger(L, 3);
	av_result_t rc;
	rc = surface->set_size(surface, width, height);
	check_result(L, rc);
	lua_pushboolean(L, 1);
	return 1;
}

static int lsurface_getsize(lua_State* L)
{
	av_surface_p surface = tosurface(L, 1);
	int width, height;
	surface->get_size(surface, &width, &height);
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}

static const struct luaL_Reg lsurface_meths[] =
{
	{ "setsize", lsurface_setsize },
	{ "getsize", lsurface_getsize },
	{ AV_NULL, AV_NULL }
};

/* system */

static int lsystem_graphics(lua_State* L)
{
	av_system_p system = tosystem(L, 1);
	new_lua_graphics(L, system->graphics);
	return 1;
}

static int lsystem_create_surface(lua_State* L)
{
	av_system_p system = tosystem(L, 1);
	av_result_t rc;
	av_bitmap_p bitmap;
	av_surface_p surface;
	const char* filename = luaL_checkstring(L, 2);

	rc = system->create_bitmap(system, &bitmap);
	check_result(L, rc);

	rc = bitmap->load(bitmap, filename);
	check_result(L, rc);

	rc = system->display->create_surface(system->display, &surface);
	check_result(L, rc);

	rc = surface->set_bitmap(surface, bitmap);
	check_result(L, rc);

	new_lua_surface(L, surface);
	return 1;
}

/* system methods */
static const struct luaL_Reg lsystem_meths[] =
{
	{ "graphics", lsystem_graphics },
	{ "createsurface", lsystem_create_surface },
	{ AV_NULL, AV_NULL }
};

static void new_lua_system(lua_State* L, av_system_p system)
{
	new_lua_object(L, (av_object_p)system);
	luaL_setfuncs(L, lsystem_meths, 0);
}

/* window */
static void _lua_pushkey(lua_State* L, av_key_t key)
{
	char* skey = AV_NULL;
	switch (key)
	{
	case AV_KEY_Backspace: skey = "backspace"; break;
	case AV_KEY_Tab: skey = "tab"; break;
	case AV_KEY_Return: skey = "return"; break;
	case AV_KEY_Escape: skey = "escape"; break;
	case AV_KEY_Space: skey = " "; break;
	case AV_KEY_Exclam: skey = "!"; break;
	case AV_KEY_Quotedbl: skey = "\""; break;
	case AV_KEY_Hash: skey = "#"; break;
	case AV_KEY_Dollar: skey = "$"; break;
	case AV_KEY_Percent: skey = "%"; break;
	case AV_KEY_Ampersand: skey = "&"; break;
	case AV_KEY_Quote: skey = "'"; break;
	case AV_KEY_Parenleft: skey = "("; break;
	case AV_KEY_Parenright: skey = ")"; break;
	case AV_KEY_Asterisk: skey = "*"; break;
	case AV_KEY_Plus: skey = "+"; break;
	case AV_KEY_Comma: skey = ","; break;
	case AV_KEY_Minus: skey = "-"; break;
	case AV_KEY_Period: skey = "."; break;
	case AV_KEY_Slash: skey = "/"; break;
	case AV_KEY_0: skey = "0"; break;
	case AV_KEY_1: skey = "1"; break;
	case AV_KEY_2: skey = "2"; break;
	case AV_KEY_3: skey = "3"; break;
	case AV_KEY_4: skey = "4"; break;
	case AV_KEY_5: skey = "5"; break;
	case AV_KEY_6: skey = "6"; break;
	case AV_KEY_7: skey = "7"; break;
	case AV_KEY_8: skey = "8"; break;
	case AV_KEY_9: skey = "9"; break;
	case AV_KEY_Colon: skey = ":"; break;
	case AV_KEY_Semicolon: skey = ";"; break;
	case AV_KEY_Less: skey = "<"; break;
	case AV_KEY_Equal: skey = "="; break;
	case AV_KEY_Greater: skey = ">"; break;
	case AV_KEY_Question: skey = "?"; break;
	case AV_KEY_At: skey = "@"; break;
	case AV_KEY_A: skey = "A"; break;
	case AV_KEY_B: skey = "B"; break;
	case AV_KEY_C: skey = "C"; break;
	case AV_KEY_D: skey = "D"; break;
	case AV_KEY_E: skey = "E"; break;
	case AV_KEY_F: skey = "F"; break;
	case AV_KEY_G: skey = "G"; break;
	case AV_KEY_H: skey = "H"; break;
	case AV_KEY_I: skey = "I"; break;
	case AV_KEY_J: skey = "J"; break;
	case AV_KEY_K: skey = "K"; break;
	case AV_KEY_L: skey = "L"; break;
	case AV_KEY_M: skey = "M"; break;
	case AV_KEY_N: skey = "N"; break;
	case AV_KEY_O: skey = "O"; break;
	case AV_KEY_P: skey = "P"; break;
	case AV_KEY_Q: skey = "Q"; break;
	case AV_KEY_R: skey = "R"; break;
	case AV_KEY_S: skey = "S"; break;
	case AV_KEY_T: skey = "T"; break;
	case AV_KEY_U: skey = "U"; break;
	case AV_KEY_V: skey = "V"; break;
	case AV_KEY_W: skey = "W"; break;
	case AV_KEY_X: skey = "X"; break;
	case AV_KEY_Y: skey = "Y"; break;
	case AV_KEY_Z: skey = "Z"; break;
	case AV_KEY_Bracketleft: skey = "["; break;
	case AV_KEY_Backslash: skey = "\\"; break;
	case AV_KEY_Bracketright: skey = "]"; break;
	case AV_KEY_Caret: skey = "^"; break;
	case AV_KEY_Underscore: skey = "_"; break;
	case AV_KEY_Backquote: skey = "`"; break;
	case AV_KEY_a: skey = "a"; break;
	case AV_KEY_b: skey = "b"; break;
	case AV_KEY_c: skey = "c"; break;
	case AV_KEY_d: skey = "d"; break;
	case AV_KEY_e: skey = "e"; break;
	case AV_KEY_f: skey = "f"; break;
	case AV_KEY_g: skey = "g"; break;
	case AV_KEY_h: skey = "h"; break;
	case AV_KEY_i: skey = "i"; break;
	case AV_KEY_j: skey = "j"; break;
	case AV_KEY_k: skey = "k"; break;
	case AV_KEY_l: skey = "l"; break;
	case AV_KEY_m: skey = "m"; break;
	case AV_KEY_n: skey = "n"; break;
	case AV_KEY_o: skey = "o"; break;
	case AV_KEY_p: skey = "p"; break;
	case AV_KEY_q: skey = "q"; break;
	case AV_KEY_r: skey = "r"; break;
	case AV_KEY_s: skey = "s"; break;
	case AV_KEY_t: skey = "t"; break;
	case AV_KEY_u: skey = "u"; break;
	case AV_KEY_v: skey = "v"; break;
	case AV_KEY_w: skey = "w"; break;
	case AV_KEY_x: skey = "x"; break;
	case AV_KEY_y: skey = "y"; break;
	case AV_KEY_z: skey = "z"; break;
	case AV_KEY_Braceleft: skey = "{"; break;
	case AV_KEY_Bar: skey = "|"; break;
	case AV_KEY_Braceright: skey = "}"; break;
	case AV_KEY_Asciitilde: skey = "~"; break;
	case AV_KEY_Delete: skey = "delete"; break;
	case AV_KEY_Left: skey = "left"; break;
	case AV_KEY_Right: skey = "right"; break;
	case AV_KEY_Up: skey = "up"; break;
	case AV_KEY_Down: skey = "down"; break;
	case AV_KEY_Insert: skey = "insert"; break;
	case AV_KEY_Home: skey = "home"; break;
	case AV_KEY_End: skey = "end"; break;
	case AV_KEY_Page_Up: skey = "page_up"; break;
	case AV_KEY_Page_Down: skey = "page_down"; break;
	case AV_KEY_Print: skey = "print"; break;
	case AV_KEY_Pause: skey = "pause"; break;
	case AV_KEY_Ok: skey = "ok"; break;
	case AV_KEY_Select: skey = "select"; break;
	case AV_KEY_Goto: skey = "goto"; break;
	case AV_KEY_Clear: skey = "clear"; break;
	case AV_KEY_Power: skey = "power"; break;
	case AV_KEY_Power2: skey = "power2"; break;
	case AV_KEY_Option: skey = "option"; break;
	case AV_KEY_Menu: skey = "menu"; break;
	case AV_KEY_Help: skey = "help"; break;
	case AV_KEY_Info: skey = "info"; break;
	case AV_KEY_Time: skey = "time"; break;
	case AV_KEY_Vendor: skey = "vendor"; break;
	case AV_KEY_Album: skey = "album"; break;
	case AV_KEY_Program: skey = "program"; break;
	case AV_KEY_Channel: skey = "channel"; break;
	case AV_KEY_Favorites: skey = "favorites"; break;
	case AV_KEY_EPG: skey = "epg"; break;
	case AV_KEY_PVR: skey = "pvr"; break;
	case AV_KEY_MHP: skey = "mhp"; break;
	case AV_KEY_Language: skey = "language"; break;
	case AV_KEY_Title: skey = "title"; break;
	case AV_KEY_Subtitle: skey = "subtitle"; break;
	case AV_KEY_Angle: skey = "angle"; break;
	case AV_KEY_Zoom: skey = "zoom"; break;
	case AV_KEY_Mode: skey = "mode"; break;
	case AV_KEY_Keyboard: skey = "keyboard"; break;
	case AV_KEY_Pc: skey = "pc"; break;
	case AV_KEY_Screen: skey = "screen"; break;
	case AV_KEY_Tv: skey = "tv"; break;
	case AV_KEY_Tv2: skey = "tv2"; break;
	case AV_KEY_Vcr2: skey = "vcr2"; break;
	case AV_KEY_Sat: skey = "sat"; break;
	case AV_KEY_Sat2: skey = "sat2"; break;
	case AV_KEY_CD: skey = "cd"; break;
	case AV_KEY_Tape: skey = "tape"; break;
	case AV_KEY_Radio: skey = "radio"; break;
	case AV_KEY_Tuner: skey = "tuner"; break;
	case AV_KEY_Player: skey = "player"; break;
	case AV_KEY_Text: skey = "text"; break;
	case AV_KEY_DVD: skey = "dvd"; break;
	case AV_KEY_Aux: skey = "aux"; break;
	case AV_KEY_MP3: skey = "mp3"; break;
	case AV_KEY_Phone: skey = "phone"; break;
	case AV_KEY_Audio: skey = "audio"; break;
	case AV_KEY_Video: skey = "video"; break;
	case AV_KEY_Internet: skey = "internet"; break;
	case AV_KEY_List: skey = "list"; break;
	case AV_KEY_Green: skey = "green"; break;
	case AV_KEY_Yellow: skey = "yellow"; break;
	case AV_KEY_Channel_Up: skey = "channel_up"; break;
	case AV_KEY_Channel_Down: skey = "channel_down"; break;
	case AV_KEY_Back: skey = "back"; break;
	case AV_KEY_Forward: skey = "forward"; break;
	case AV_KEY_First: skey = "first"; break;
	case AV_KEY_Last: skey = "last"; break;
	case AV_KEY_Volume_Up: skey = "volume_up"; break;
	case AV_KEY_Volume_Down: skey = "volume_down"; break;
	case AV_KEY_Mute: skey = "mute"; break;
	case AV_KEY_AB: skey = "ab"; break;
	case AV_KEY_Playpause: skey = "play_pause"; break;
	case AV_KEY_PLAY: skey = "play"; break;
	case AV_KEY_Stop: skey = "stop"; break;
	case AV_KEY_Restart: skey = "restart"; break;
	case AV_KEY_Slow: skey = "slow"; break;
	case AV_KEY_Fast: skey = "fast"; break;
	case AV_KEY_Record: skey = "record"; break;
	case AV_KEY_Eject: skey = "eject"; break;
	case AV_KEY_Shuffle: skey = "shuffle"; break;
	case AV_KEY_Rewind: skey = "rewind"; break;
	case AV_KEY_Fastforward: skey = "fast_forward"; break;
	case AV_KEY_Previous: skey = "previous"; break;
	case AV_KEY_Next: skey = "next"; break;
	case AV_KEY_Begin: skey = "begin"; break;
	case AV_KEY_Digits: skey = "digits"; break;
	case AV_KEY_Teen: skey = "teen"; break;
	case AV_KEY_Twen: skey = "twen"; break;
	case AV_KEY_Break: skey = "break"; break;
	case AV_KEY_Exit: skey = "exit"; break;
	case AV_KEY_Setup: skey = "setup"; break;
	case AV_KEY_F1: skey = "f1"; break;
	case AV_KEY_F2: skey = "f2"; break;
	case AV_KEY_F3: skey = "f3"; break;
	case AV_KEY_F4: skey = "f4"; break;
	case AV_KEY_F5: skey = "f5"; break;
	case AV_KEY_F6: skey = "f6"; break;
	case AV_KEY_F7: skey = "f7"; break;
	case AV_KEY_F8: skey = "f8"; break;
	case AV_KEY_F9: skey = "f9"; break;
	case AV_KEY_F10: skey = "f10"; break;
	case AV_KEY_F11: skey = "f11"; break;
	case AV_KEY_F12: skey = "f12"; break;
	case AV_KEY_Numlock: skey = "num_lock"; break;
	case AV_KEY_Capslock: skey = "caps_lock"; break;
	case AV_KEY_Scrolllock: skey = "scroll_lock"; break;
	case AV_KEY_RightShift: skey = "right_shift"; break;
	case AV_KEY_LeftShift: skey = "left_shift"; break;
	case AV_KEY_RightControl: skey = "right_control"; break;
	case AV_KEY_LeftControl: skey = "left_control"; break;
	case AV_KEY_RightAlt: skey = "right_alt"; break;
	case AV_KEY_LeftAlt: skey = "left_alt"; break;
	default: skey = "unknown_key"; break;
	}
	lua_pushstring(L, skey);
}

static void _lua_push_mouse_button(lua_State* L, av_event_mouse_button_t button)
{
	char* skey = AV_NULL;
	switch (button)
	{
		case AV_MOUSE_BUTTON_LEFT: skey = "left-button"; break;
		case AV_MOUSE_BUTTON_RIGHT: skey = "right-button"; break;
		case AV_MOUSE_BUTTON_MIDDLE: skey = "middle-button"; break;
		case AV_MOUSE_BUTTON_WHEEL: skey = "wheel-button"; break;
		case AV_MOUSE_BUTTON_X1: skey = "x1-button"; break;
		case AV_MOUSE_BUTTON_X2: skey = "x2-button"; break;
		default: skey = "unknown_key"; break;
	}
	lua_pushstring(L, skey);
}

static av_bool_t window_on_key_down(struct av_window* self, av_key_t key)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onkeydown");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		_lua_pushkey(L, key);
		lua_call(L, 1, 1);
		result = lua_toboolean(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t window_on_key_up(struct av_window* self, av_key_t key)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onkeyup");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		_lua_pushkey(L, key);
		lua_call(L, 1, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_move(av_window_p self, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmousemove");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 2, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_enter(av_window_p self, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmouseenter");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 2, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_hover(av_window_p self, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmousehover");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 2, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_leave(av_window_p self, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmouseleave");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 2, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_button_up(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmousebuttonup");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		_lua_push_mouse_button(L, button);
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 3, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static av_bool_t av_window_on_mouse_button_down(av_window_p self, av_event_mouse_button_t button, int x, int y)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	av_bool_t result = AV_FALSE;
	lua_pushliteral(L, "onmousebuttondown");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		_lua_push_mouse_button(L, button);
		lua_pushinteger(L, x);
		lua_pushinteger(L, y);
		lua_call(L, 3, 1);
		result = AV_BOOL(lua_toboolean(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return result;
}

static int lwindow_detach(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	window->detach(window);
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int lwindow_setpos(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	av_result_t rc;
	window->get_rect(window, &rect);
	rect.x = (int)luaL_checkinteger(L, 2);
	rect.y = (int)luaL_checkinteger(L, 3);
	rc = window->set_rect(window, &rect);
	check_result(L, rc)
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int lwindow_getpos(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	window->get_rect(window, &rect);
	lua_pushinteger(L, rect.x);
	lua_pushinteger(L, rect.y);
	return 2;
}

static int lwindow_setsize(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	av_result_t rc;
	window->get_rect(window, &rect);
	rect.w = (int)luaL_checkinteger(L, 2);
	rect.h = (int)luaL_checkinteger(L, 3);
	rc = window->set_rect(window, &rect);
	check_result(L, rc)
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int lwindow_getsize(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	window->get_rect(window, &rect);
	lua_pushinteger(L, rect.w);
	lua_pushinteger(L, rect.h);
	return 2;
}

static int lwindow_setrect(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	av_result_t rc;
	avlua_torect(L, 2, &rect);
	rc = window->set_rect(window, &rect);
	check_result(L, rc)
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int lwindow_getrect(lua_State* L)
{
	av_window_p window = towindow(L, 1);
	av_rect_t rect;
	window->get_rect(window, &rect);
	avlua_rect_new(L, &rect);
	return 1;
}

static const struct luaL_Reg lwindow_meths[] =
{
	{ "detach", lwindow_detach },
	{ "setpos", lwindow_setpos },
	{ "getpos", lwindow_getpos },
	{ "setsize", lwindow_setsize },
	{ "getsize", lwindow_getsize },
	{ "setrect", lwindow_setrect },
	{ "getrect", lwindow_getrect },
	{ AV_NULL, AV_NULL }
};

/* visible */
static void visible_on_draw(struct _av_visible_t* self, av_graphics_p graphics)
{
	lua_State* L = avlua_push_object((av_object_p)self);
	lua_pushliteral(L, "ondraw");
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1))
	{
		new_lua_graphics(L, graphics);
		lua_call(L, 1, 0);
	}
	lua_pop(L, 1);
}

static int lvisible_createwindow(lua_State* L)
{
	av_visible_p visible = tovisible(L, 1);
	av_visible_p child;
	av_result_t rc;
	const char *kind = luaL_optstring(L, 2, "visible");
	rc = visible->create_child(visible, kind, &child);
	check_result(L, rc)
	new_lua_visible(L, child);
	return 1;
}

static int lvisible_system(lua_State* L)
{
	av_visible_p visible = tovisible(L, 1);
	new_lua_system(L, visible->system);
	return 1;
}

static int lvisible_set_surface(lua_State* L)
{
	av_visible_p visible = tovisible(L, 1);
	av_surface_p surface = tosurface(L, 2);
	visible->set_surface(visible, surface);
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static const struct luaL_Reg lvisible_meths[] =
{
	{ "createwindow", lvisible_createwindow },
	{ "system", lvisible_system }, // FIXME: Convert to property
	{ "setsurface", lvisible_set_surface},
	{ AV_NULL, AV_NULL }
};

static int lsprite_set_size(lua_State* L)
{
	av_sprite_p sprite = tosprite(L, 1);
	int frame_width = (int)luaL_checkinteger(L, 2);
	int frame_height = (int)luaL_checkinteger(L, 3);
	sprite->set_size(sprite, frame_width, frame_height);
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int lsprite_get_size(lua_State* L)
{
	av_sprite_p sprite = tosprite(L, 1);
	int frame_width, frame_height;
	sprite->get_size(sprite, &frame_width, &frame_height);
	lua_pushinteger(L, frame_width);
	lua_pushinteger(L, frame_height);
	return 2;
}

static int lsprite_get_frames_count(lua_State* L)
{
	av_sprite_p sprite = tosprite(L, 1);
	lua_pushinteger(L, sprite->get_frames_count(sprite));
	return 1;
}

static int lsprite_set_current_frame(lua_State* L)
{
	av_sprite_p* psprite = (av_sprite_p *)lua_touserdata(L, 1);
	av_sprite_p sprite = *psprite;
	int frame = (int)luaL_checkinteger(L, 2);
	sprite->set_current_frame(sprite, frame);
	return 0;
}

static int lsprite_get_current_frame(lua_State* L)
{
	av_sprite_p* psprite = (av_sprite_p *)lua_touserdata(L, 1);
	av_sprite_p sprite = *psprite;
	lua_pushinteger(L, sprite->get_current_frame(sprite));
	return 1;
}

static int lsprite_set_sequence(lua_State* L)
{
	av_sprite_p* psprite = (av_sprite_p *)lua_touserdata(L, 1);
	av_sprite_p sprite = *psprite;
	int* seq = luatable_tointarray(L, 2);
	int duration = (int)lua_tointeger(L, 3);
	av_bool_t loop = lua_toboolean(L, 4);
	sprite->set_sequence(sprite, &seq[1], seq[0], duration, loop);
	free(seq);
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static const struct luaL_Reg lsprite_meths[] =
{
	{ "setsize", lsprite_set_size },
	{ "getsize", lsprite_get_size },
	{ "getframescount", lsprite_get_frames_count },
	{ "setcurrentframe", lsprite_set_current_frame },
	{ "getcurrentframe", lsprite_get_current_frame },
	{ "setsequence", lsprite_set_sequence },
	{ AV_NULL, AV_NULL }
};

static void new_lua_surface(lua_State* L, av_surface_p surface)
{
	new_lua_object(L, (av_object_p)surface);
	luaL_setfuncs(L, lsurface_meths, 0);
}

static void new_lua_visible(lua_State* L, av_visible_p visible)
{
	new_lua_object(L, (av_object_p)visible);
	((av_window_p)visible)->on_key_down           = window_on_key_down;
	((av_window_p)visible)->on_key_up             = window_on_key_up;
	((av_window_p)visible)->on_mouse_move         = av_window_on_mouse_move;
	((av_window_p)visible)->on_mouse_enter        = av_window_on_mouse_enter;
	((av_window_p)visible)->on_mouse_hover        = av_window_on_mouse_hover;
	((av_window_p)visible)->on_mouse_leave        = av_window_on_mouse_leave;
	((av_window_p)visible)->on_mouse_button_up    = av_window_on_mouse_button_up;
	((av_window_p)visible)->on_mouse_button_down  = av_window_on_mouse_button_down;
	visible->on_draw = visible_on_draw;
	luaL_setfuncs(L, lwindow_meths, 0);
	luaL_setfuncs(L, lvisible_meths, 0);
	if (O_is_a(visible, "sprite"))
	{
		luaL_setfuncs(L, lsprite_meths, 0);
	}
}

/* graphics_surface */
static int avlua_graphics_surface_create_pattern(lua_State* L)
{
	av_result_t rc;
	av_graphics_surface_p self = tographics_surface(L, 1);
	av_graphics_pattern_p graphics_pattern;
	if (AV_OK != (rc = self->create_pattern(self, &graphics_pattern)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
	}
	new_lua_graphics_pattern(L, graphics_pattern);
	return 1;
}

static int avlua_graphics_surface_save(lua_State* L)
{
	av_result_t rc;
	av_graphics_surface_p self = tographics_surface(L, 1);
	const char* filename = luaL_checkstring(L, 2);
	if (AV_OK != (rc = self->save(self, filename)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
	}
	lua_pushboolean(L, 1);
	return 1;
}

static const struct luaL_Reg lgraphics_surface_meths[] =
{
	{ "create_pattern", avlua_graphics_surface_create_pattern },
	{ "save", avlua_graphics_surface_save },
	{ AV_NULL, AV_NULL }
};

static void new_lua_graphics_surface(lua_State* L, av_graphics_surface_p graphics_surface)
{
	new_lua_object(L, (av_object_p)graphics_surface);
	// luaL_setfuncs(L, lsurface_meths, 0);
	luaL_setfuncs(L, lgraphics_surface_meths, 0);
}

/* graphics */
static int avlua_graphics_create_surface(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_surface_p graphics_surface;
	if (lua_isstring(L, 1))
	{
		const char* filename = lua_tostring(L, 1);
		if (AV_OK != (rc = self->create_surface_from_file(self, filename, &graphics_surface)))
		{
			lua_pushnil(L);
			lua_pushinteger(L, (int)rc);
			return 2;
		}
	}
	else
	{
		int width = (int)luaL_checkinteger(L, 2);
		int height = (int)luaL_checkinteger(L, 3);

		if (AV_OK != (rc = self->create_surface(self, width, height, &graphics_surface)))
		{
			lua_pushnil(L);
			lua_pushinteger(L, (int)rc);
			return 2;
		}
	}
	new_lua_graphics_surface(L, graphics_surface);
	return 1;
}

static int avlua_graphics_begin(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_surface_p graphics_surface = tographics_surface(L, 2);
	if (AV_OK != (rc = self->begin(self, graphics_surface)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, (int)rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_end(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->end(self);
	return 0;
}

static int avlua_graphics_push_group(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	const char* scontent = luaL_checkstring(L, 2);
	av_graphics_content_t content = AV_CONTENT_COLOR_ALPHA;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("COLOR_ALPHA", scontent)))
	{
		if (0 == av_strcasecmp("COLOR", scontent))
			content = AV_CONTENT_COLOR;
		else if (0 == av_strcasecmp("ALPHA", scontent))
			content = AV_CONTENT_ALPHA;
		else luaL_argerror(L, 2,
			"Invalid graphics content, expected `color_alpha', `color' or `alpha'");
	}

	if (AV_OK != (rc = self->push_group(self, content)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_pop_group(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_pattern_p graphics_pattern;
	if (AV_OK != (rc = self->pop_group(self, &graphics_pattern)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}

	new_lua_graphics_pattern(L, graphics_pattern);
	return 1;
}

static int avlua_graphics_create_pattern_rgba(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_pattern_p graphics_pattern;
	if (AV_OK != (rc = self->create_pattern_rgba(self,
		luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		&graphics_pattern)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	new_lua_graphics_pattern(L, graphics_pattern);
	return 1;
}

static int avlua_graphics_create_pattern_linear(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_pattern_p graphics_pattern;
	if (AV_OK != (rc = self->create_pattern_linear(self,
		luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		&graphics_pattern)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	new_lua_graphics_pattern(L, graphics_pattern);
	return 1;
}

static int avlua_graphics_create_pattern_radial(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_graphics_pattern_p graphics_pattern;
	if (AV_OK != (rc = self->create_pattern_radial(self,
		luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		luaL_checknumber(L, 6), luaL_checknumber(L, 6),
		&graphics_pattern)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	new_lua_graphics_pattern(L, graphics_pattern);
	return 1;
}

static int avlua_graphics_set_pattern(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	av_graphics_pattern_p graphics_pattern = tographics_pattern(L, 2);
	self->set_pattern(self, graphics_pattern);
	return 0;
}

static int avlua_graphics_set_clip(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_rect_t rect;
	avlua_torect(L, 2, &rect);
	if (AV_OK != (rc = self->set_clip(self, &rect)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_get_clip(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_rect_t rect;
	if (AV_OK != (rc = self->get_clip(self, &rect)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	avlua_rect_new(L, &rect);
	return 1;
}

static int avlua_graphics_save(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->save(self)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_restore(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->restore(self)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_move_to(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->move_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	return 0;
}

static int avlua_graphics_rel_move_to(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->rel_move_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	return 0;
}

static int avlua_graphics_line_to(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->line_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_rel_line_to(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->rel_line_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_curve_to(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->curve_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		luaL_checknumber(L, 6), luaL_checknumber(L, 7))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_rel_curve_to(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->rel_curve_to(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		luaL_checknumber(L, 6), luaL_checknumber(L, 7))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_arc(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->arc(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		luaL_checknumber(L, 6))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_arc_negative(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->arc_negative(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5),
		luaL_checknumber(L, 6))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_close_path(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->close_path(self)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_rectangle(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_rect_t rect;
	avlua_torect(L, 2, &rect);
	if (AV_OK != (rc = self->rectangle(self, &rect)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_set_line_width(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->set_line_width(self, luaL_checknumber(L, 2))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_set_line_cap(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	const char* scap = luaL_checkstring(L, 2);
	av_line_cap_t cap = AV_LINE_CAP_ROUND;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("ROUND", scap)))
	{
		if (0 == av_strcasecmp("SQUARE", scap))
			cap = AV_LINE_CAP_SQUARE;
		else if (0 == av_strcasecmp("BUTT", scap))
			cap = AV_LINE_CAP_BUTT;
		else luaL_argerror(L, 2, "Invalid line cap, expected `round', `square' or `butt'");
	}

	if (AV_OK != (rc = self->set_line_cap(self, cap)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}

	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_set_line_join(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	const char* sjoin = luaL_checkstring(L, 2);
	av_line_join_t join = AV_LINE_JOIN_ROUND;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("ROUND", sjoin)))
	{
		if (0 == av_strcasecmp("BEVEL", sjoin))
			join = AV_LINE_JOIN_BEVEL;
		else if (0 == av_strcasecmp("MITER", sjoin))
			join = AV_LINE_JOIN_MITER;
		else luaL_argerror(L, 2, "Invalid line join, expected `round', `miter' or `bevel'");
	}

	if (AV_OK != (rc = self->set_line_join(self, join)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}

	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_stroke(lua_State* L)
{
	av_result_t rc;
	av_bool_t preserve;
	av_graphics_p self = tographics(L, 1);
	preserve = lua_toboolean(L, 2);

	if (AV_OK != (rc = self->stroke(self, preserve)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_fill(lua_State* L)
{
	av_result_t rc;
	av_bool_t preserve;
	av_graphics_p self = tographics(L, 1);
	preserve = lua_toboolean(L, 2);

	if (AV_OK != (rc = self->fill(self, preserve)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_paint(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	double alpha = luaL_checknumber(L, 2);

	if (AV_OK != (rc = self->paint(self, alpha)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_flush(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->flush(self)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_set_offset(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->set_offset(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	return 0;
}

static int avlua_graphics_get_offset(lua_State* L)
{
	double x_offs, y_offs;
	av_graphics_p self = tographics(L, 1);
	self->get_offset(self, &x_offs, &y_offs);
	lua_pushnumber(L, x_offs);
	lua_pushnumber(L, y_offs);
	return 2;
}

static int avlua_graphics_set_scale(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->set_scale(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	return 0;
}

static int avlua_graphics_set_color_rgba(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->set_color_rgba(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3),
		luaL_checknumber(L, 4), luaL_checknumber(L, 5));
	return 0;
}

static int avlua_graphics_show_text(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->show_text(self, luaL_checkstring(L, 2));
	return 0;
}

static int avlua_graphics_text_path(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->text_path(self, luaL_checkstring(L, 2));
	return 0;
}

static int avlua_graphics_show_image(lua_State* L)
{
	av_graphics_p self = tographics(L, 1);
	self->show_image(self, luaL_checknumber(L, 2), luaL_checknumber(L, 3), tographics_surface(L, 4));
	return 0;
}

static int avlua_graphics_get_text_extents(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	int width;
	int height;
	int xbearing;
	int ybearing;
	int xadvance;
	int yadvance;

	if (AV_OK != (rc = self->get_text_extents(self, luaL_checkstring(L, 2), &width, &height, &xbearing, &ybearing, &xadvance, &yadvance)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_newtable(L);
	lua_pushinteger(L, width);
	lua_setfield(L, -2, "width");
	lua_pushinteger(L, height);
	lua_setfield(L, -2, "height");
	lua_pushinteger(L, xbearing);
	lua_setfield(L, -2, "xbearing");
	lua_pushinteger(L, ybearing);
	lua_setfield(L, -2, "ybearing");
	lua_pushinteger(L, xadvance);
	lua_setfield(L, -2, "xadvance");
	lua_pushinteger(L, yadvance);
	lua_setfield(L, -2, "yadvance");
	return 1;
}

static int avlua_graphics_set_font_face(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	av_font_slant_t slant = AV_FONT_SLANT_NORMAL;
	av_font_weight_t weight = AV_FONT_WEIGHT_NORMAL;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("NORMAL", luaL_checkstring(L, 3))))
	{
		if (0 == av_strcasecmp("ITALIC", luaL_checkstring(L, 3)))
			slant = AV_FONT_SLANT_ITALIC;
		else if (0 == av_strcasecmp("OBLIQUE", luaL_checkstring(L, 3)))
			slant = AV_FONT_SLANT_OBLIQUE;
		else luaL_argerror(L, 3, "Invalid font slant, expected `normal', `italic' or `oblique'");
	}

	if (lua_gettop(L) > 2 && (0 != av_strcasecmp("NORMAL", luaL_checkstring(L, 4))))
	{
		if (0 == av_strcasecmp("BOLD", luaL_checkstring(L, 4)))
			weight = AV_FONT_WEIGHT_BOLD;
		else luaL_argerror(L, 4, "Invalid font weight, expected `normal', `bold'");
	}

	if (AV_OK != (rc = self->set_font_face(self, luaL_checkstring(L, 2), slant, weight)))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

static int avlua_graphics_set_font_size(lua_State* L)
{
	av_result_t rc;
	av_graphics_p self = tographics(L, 1);
	if (AV_OK != (rc = self->set_font_size(self, (int)luaL_checkinteger(L, 2))))
	{
		lua_pushnil(L);
		lua_pushinteger(L, rc);
		return 2;
	}
	lua_pushboolean(L, AV_TRUE);
	return 1;
}

/* graphics methods */
static const struct luaL_Reg lgraphics_meths[] =
{
	{ "createsurface", avlua_graphics_create_surface },
	{ "begin", avlua_graphics_begin },
	{ "end_", avlua_graphics_end },
	{ "pushgroup", avlua_graphics_push_group },
	{ "popgroup", avlua_graphics_pop_group },
	{ "createpatternrgba", avlua_graphics_create_pattern_rgba },
	{ "createpatternlinear", avlua_graphics_create_pattern_linear },
	{ "createpatternradial", avlua_graphics_create_pattern_radial },
	{ "setpattern", avlua_graphics_set_pattern },
	{ "setclip", avlua_graphics_set_clip },
	{ "getclip", avlua_graphics_get_clip },
	{ "save", avlua_graphics_save },
	{ "restore", avlua_graphics_restore },
	{ "moveto", avlua_graphics_move_to },
	{ "relmoveto", avlua_graphics_rel_move_to },
	{ "lineto", avlua_graphics_line_to },
	{ "rellineto", avlua_graphics_rel_line_to },
	{ "curveto", avlua_graphics_curve_to },
	{ "relcurveto", avlua_graphics_rel_curve_to },
	{ "arc", avlua_graphics_arc },
	{ "arcnegative", avlua_graphics_arc_negative },
	{ "closepath", avlua_graphics_close_path },
	{ "rectangle", avlua_graphics_rectangle },
	{ "setlinewidth", avlua_graphics_set_line_width },
	{ "setlinecap", avlua_graphics_set_line_cap },
	{ "setlinejoin", avlua_graphics_set_line_join },
	{ "stroke", avlua_graphics_stroke },
	{ "fill", avlua_graphics_fill },
	{ "paint", avlua_graphics_paint },
	{ "flush", avlua_graphics_flush },
	{ "setoffset", avlua_graphics_set_offset },
	{ "getoffset", avlua_graphics_get_offset },
	{ "setscale", avlua_graphics_set_scale },
	{ "setcolorrgba", avlua_graphics_set_color_rgba },
	{ "textpath", avlua_graphics_text_path },
	{ "showtext", avlua_graphics_show_text },
	{ "showimage", avlua_graphics_show_image },
	{ "gettextextents", avlua_graphics_get_text_extents },
	{ "setfontface", avlua_graphics_set_font_face },
	{ "setfontsize", avlua_graphics_set_font_size },
	{ AV_NULL, AV_NULL }
};

static void new_lua_graphics(lua_State* L, av_graphics_p graphics)
{
	new_lua_object(L, (av_object_p)graphics);
	luaL_setfuncs(L, lgraphics_meths, 0);
}

/* graphics_pattern */
static int avlua_graphics_pattern_add_stop_rgba(lua_State* L)
{
	av_graphics_pattern_p pattern = tographics_pattern(L, 1);
	pattern->add_stop_rgba(pattern,
		luaL_checknumber(L, 2),  /* offset */
		luaL_checknumber(L, 3),  /* red */
		luaL_checknumber(L, 4),  /* green */
		luaL_checknumber(L, 5),  /* blue */
		luaL_checknumber(L, 6)); /* alpha */
	return 0;
}

static int avlua_graphics_pattern_set_extend(lua_State* L)
{
	av_graphics_pattern_p pattern = tographics_pattern(L, 1);
	const char* sextend = luaL_checkstring(L, 2);
	av_graphics_extend_t extend = AV_EXTEND_NONE;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("NONE", sextend)))
	{
		if (0 == av_strcasecmp("REPEAT", sextend))
			extend = AV_EXTEND_REPEAT;
		else if (0 == av_strcasecmp("REFLECT", sextend))
			extend = AV_EXTEND_REFLECT;
		else if (0 == av_strcasecmp("PAD", sextend))
			extend = AV_EXTEND_PAD;
		else luaL_argerror(L, 2,
			"Invalid extend value, expected `none', `repeat', `reflect' or `pad'");
	}
	pattern->set_extend(pattern, extend);
	return 0;
}

static int avlua_graphics_pattern_set_filter(lua_State* L)
{
	av_graphics_pattern_p pattern = tographics_pattern(L, 1);
	const char* sfilter = luaL_checkstring(L, 2);
	av_filter_t filter = AV_FILTER_FAST;

	if (lua_gettop(L) > 1 && (0 != av_strcasecmp("FAST", sfilter)))
	{
		if (0 == av_strcasecmp("GOOD", sfilter))
			filter = AV_FILTER_GOOD;
		else if (0 == av_strcasecmp("BEST", sfilter))
			filter = AV_FILTER_BEST;
		else if (0 == av_strcasecmp("NEAREST", sfilter))
			filter = AV_FILTER_NEAREST;
		else if (0 == av_strcasecmp("BILINEAR", sfilter))
			filter = AV_FILTER_BILINEAR;
		else if (0 == av_strcasecmp("GAUSSIAN", sfilter))
			filter = AV_FILTER_GAUSSIAN;
		else luaL_argerror(L, 2,
			"Invalid filter value, expected `fast', `good', `best', `nearest', `bilinear' or  `gaussian'");
	}
	pattern->set_filter(pattern, filter);
	return 0;
}

static const struct luaL_Reg lgraphics_pattern_meths[] =
{
	{ "add_stop_rgba", avlua_graphics_pattern_add_stop_rgba },
	{ "set_extend", avlua_graphics_pattern_set_extend },
	{ "set_filter", avlua_graphics_pattern_set_filter },
	{ AV_NULL, AV_NULL }
};

static void new_lua_graphics_pattern(lua_State* L, av_graphics_pattern_p graphics_pattern)
{
	new_lua_object(L, (av_object_p)graphics_pattern);
	luaL_setfuncs(L, lgraphics_pattern_meths, 0);
}

/* avgl */
static int lavgl_create(lua_State* L)
{
	av_display_config_t display_config;
	luaL_checktype(L, 1, LUA_TTABLE);
	av_memset(&display_config, 0, sizeof(av_display_config_t));
	lua_getfield(L, 1, "width");
	lua_getfield(L, 1, "height");
	lua_getfield(L, 1, "scale_x");
	lua_getfield(L, 1, "scale_y");
	display_config.width = (int)luaL_checkinteger(L, -4);
	display_config.height = (int)luaL_checkinteger(L, -3);
	display_config.scale_x = (int)luaL_optinteger(L, -2, 1);
	display_config.scale_y = (int)luaL_optinteger(L, -1, 1);
	lua_pop(L, 4);
	new_lua_visible(L, avgl_create(&display_config));
	return 1;
}

static int lavgl_loop(lua_State* L)
{
	avgl_loop();
	return 0;
}

static int lavgl_step(lua_State* L)
{
	avgl_step();
	return 0;
}

/* AVGL main functions */
static const struct luaL_Reg lavgl_funcs[] =
{
	{ "create", lavgl_create },
	{ "loop", lavgl_loop },
	{ "step", lavgl_step },
	{ AV_NULL, AV_NULL }
};

LAVGL_API int luaopen_lavgl(lua_State *L)
{
	lua_newtable(L);
	luaL_setfuncs(L, lavgl_funcs, 0);
	// lua_setglobal(L, LAVGL_PACKAGE);
	return 1;
}
