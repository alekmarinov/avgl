/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_keys.h                                          */
/*                                                                   */
/*********************************************************************/

/*! \file av_keys.h
*   \brief Defines key codes
*/

#ifndef __AV_KEYS_H
#define __AV_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Enumeration of AVGL key codes (symbols) */
typedef enum
{
	AV_KEY_Backspace=8,          /* 0x08   */
	AV_KEY_Tab,                  /* 0x09   */
	AV_KEY_Return=13,            /* 0x0D   */
	AV_KEY_Escape=27,            /* 0x1B   */
	AV_KEY_Space=32,             /* 0x20   */
	AV_KEY_Exclam,               /* 0x21 ! */
	AV_KEY_Quotedbl,             /* 0x22 " */
	AV_KEY_Hash,                 /* 0x23 # */
	AV_KEY_Dollar,               /* 0x24 $ */
	AV_KEY_Percent,              /* 0x25 % */
	AV_KEY_Ampersand,            /* 0x26 & */
	AV_KEY_Quote,                /* 0x27 ' */
	AV_KEY_Parenleft,            /* 0x28 ( */
	AV_KEY_Parenright,           /* 0x29 ) */
	AV_KEY_Asterisk,             /* 0x2A * */
	AV_KEY_Plus,                 /* 0x2B + */
	AV_KEY_Comma,                /* 0x2C , */
	AV_KEY_Minus,                /* 0x2D - */
	AV_KEY_Period,               /* 0x2E . */
	AV_KEY_Slash,                /* 0x2F / */
	AV_KEY_0=48,                 /* 0x30 0 */
	AV_KEY_1,                    /* 0x31 1 */
	AV_KEY_2,                    /* 0x32 2 */
	AV_KEY_3,                    /* 0x33 3 */
	AV_KEY_4,                    /* 0x34 4 */
	AV_KEY_5,                    /* 0x35 5 */
	AV_KEY_6,                    /* 0x36 6 */
	AV_KEY_7,                    /* 0x37 7 */
	AV_KEY_8,                    /* 0x38 8 */
	AV_KEY_9,                    /* 0x39 9 */
	AV_KEY_Colon,                /* 0x3A : */
	AV_KEY_Semicolon,            /* 0x3B ; */
	AV_KEY_Less,                 /* 0x3C < */
	AV_KEY_Equal,                /* 0x3D = */
	AV_KEY_Greater,              /* 0x3E > */
	AV_KEY_Question,             /* 0x3F ? */
	AV_KEY_At,                   /* 0x40 @ */
	AV_KEY_A,                    /* 0x41 A */
	AV_KEY_B,                    /* 0x42 B */
	AV_KEY_C,                    /* 0x43 C */
	AV_KEY_D,                    /* 0x44 D */
	AV_KEY_E,                    /* 0x45 E */
	AV_KEY_F,                    /* 0x46 F */
	AV_KEY_G,                    /* 0x47 G */
	AV_KEY_H,                    /* 0x48 H */
	AV_KEY_I,                    /* 0x49 I */
	AV_KEY_J,                    /* 0x4A J */
	AV_KEY_K,                    /* 0x4B K */
	AV_KEY_L,                    /* 0x4C L */
	AV_KEY_M,                    /* 0x4D M */
	AV_KEY_N,                    /* 0x4E N */
	AV_KEY_O,                    /* 0x4F O */
	AV_KEY_P,                    /* 0x50 P */
	AV_KEY_Q,                    /* 0x51 Q */
	AV_KEY_R,                    /* 0x52 R */
	AV_KEY_S,                    /* 0x53 S */
	AV_KEY_T,                    /* 0x54 T */
	AV_KEY_U,                    /* 0x55 U */
	AV_KEY_V,                    /* 0x56 V */
	AV_KEY_W,                    /* 0x57 W */
	AV_KEY_X,                    /* 0x58 X */
	AV_KEY_Y,                    /* 0x59 Y */
	AV_KEY_Z,                    /* 0x5A Z */
	AV_KEY_Bracketleft,          /* 0x5B [ */
	AV_KEY_Backslash,            /* 0x5C \ */
	AV_KEY_Bracketright,         /* 0x5D ] */
	AV_KEY_Caret,                /* 0x5E ^ */
	AV_KEY_Underscore,           /* 0x5F _ */
	AV_KEY_Backquote,            /* 0x60 ` */
	AV_KEY_a,                    /* 0x61 a */
	AV_KEY_b,                    /* 0x62 b */
	AV_KEY_c,                    /* 0x63 c */
	AV_KEY_d,                    /* 0x64 d */
	AV_KEY_e,                    /* 0x65 e */
	AV_KEY_f,                    /* 0x66 f */
	AV_KEY_g,                    /* 0x67 g */
	AV_KEY_h,                    /* 0x68 h */
	AV_KEY_i,                    /* 0x69 i */
	AV_KEY_j,                    /* 0x6A j */
	AV_KEY_k,                    /* 0x6B k */
	AV_KEY_l,                    /* 0x6C l */
	AV_KEY_m,                    /* 0x6D m */
	AV_KEY_n,                    /* 0x6E n */
	AV_KEY_o,                    /* 0x6F o */
	AV_KEY_p,                    /* 0x70 p */
	AV_KEY_q,                    /* 0x71 q */
	AV_KEY_r,                    /* 0x72 r */
	AV_KEY_s,                    /* 0x73 s */
	AV_KEY_t,                    /* 0x74 t */
	AV_KEY_u,                    /* 0x75 u */
	AV_KEY_v,                    /* 0x76 v */
	AV_KEY_w,                    /* 0x77 w */
	AV_KEY_x,                    /* 0x78 x */
	AV_KEY_y,                    /* 0x79 y */
	AV_KEY_z,                    /* 0x7A z */
	AV_KEY_Braceleft,            /* 0x7B { */
	AV_KEY_Bar,                  /* 0x7C | */
	AV_KEY_Braceright,           /* 0x7D } */
	AV_KEY_Asciitilde,           /* 0x7E ~ */
	AV_KEY_Delete,               /* 0x7F   */

	AV_KEY_Left,
	AV_KEY_Right,
	AV_KEY_Up,
	AV_KEY_Down,
	AV_KEY_Insert,
	AV_KEY_Home,
	AV_KEY_End,
	AV_KEY_Page_Up,
	AV_KEY_Page_Down,
	AV_KEY_Print,
	AV_KEY_Pause,
	AV_KEY_Ok,
	AV_KEY_Select,
	AV_KEY_Goto,
	AV_KEY_Clear,
	AV_KEY_Power,
	AV_KEY_Power2,
	AV_KEY_Option,
	AV_KEY_Menu,
	AV_KEY_Help,
	AV_KEY_Info,
	AV_KEY_Time,
	AV_KEY_Vendor,
	AV_KEY_Album,
	AV_KEY_Program,
	AV_KEY_Channel,
	AV_KEY_Favorites,
	AV_KEY_EPG,
	AV_KEY_PVR,
	AV_KEY_MHP,
	AV_KEY_Language,
	AV_KEY_Title,
	AV_KEY_Subtitle,
	AV_KEY_Angle,
	AV_KEY_Zoom,
	AV_KEY_Mode,
	AV_KEY_Keyboard,
	AV_KEY_Pc,
	AV_KEY_Screen,
	AV_KEY_Tv,
	AV_KEY_Tv2,
	AV_KEY_Vcr2,
	AV_KEY_Sat,
	AV_KEY_Sat2,
	AV_KEY_CD,
	AV_KEY_Tape,
	AV_KEY_Radio,
	AV_KEY_Tuner,
	AV_KEY_Player,
	AV_KEY_Text,
	AV_KEY_DVD,
	AV_KEY_Aux,
	AV_KEY_MP3,
	AV_KEY_Phone,
	AV_KEY_Audio,
	AV_KEY_Video,
	AV_KEY_Internet,
	AV_KEY_List,
	AV_KEY_Green,
	AV_KEY_Yellow,
	AV_KEY_Channel_Up,
	AV_KEY_Channel_Down,
	AV_KEY_Back,
	AV_KEY_Forward,
	AV_KEY_First,
	AV_KEY_Last,
	AV_KEY_Volume_Up,
	AV_KEY_Volume_Down,
	AV_KEY_Mute,
	AV_KEY_AB,
	AV_KEY_Playpause,
	AV_KEY_PLAY,
	AV_KEY_Stop,
	AV_KEY_Restart,
	AV_KEY_Slow,
	AV_KEY_Fast,
	AV_KEY_Record,
	AV_KEY_Eject,
	AV_KEY_Shuffle,
	AV_KEY_Rewind,
	AV_KEY_Fastforward,
	AV_KEY_Previous,
	AV_KEY_Next,
	AV_KEY_Begin,
	AV_KEY_Digits,
	AV_KEY_Teen,
	AV_KEY_Twen,
	AV_KEY_Break,
	AV_KEY_Exit,
	AV_KEY_Setup,
	AV_KEY_F1,
	AV_KEY_F2,
	AV_KEY_F3,
	AV_KEY_F4,
	AV_KEY_F5,
	AV_KEY_F6,
	AV_KEY_F7,
	AV_KEY_F8,
	AV_KEY_F9,
	AV_KEY_F10,
	AV_KEY_F11,
	AV_KEY_F12,
	AV_KEY_Numlock,
	AV_KEY_Capslock,
	AV_KEY_Scrolllock,
	AV_KEY_RightShift,
	AV_KEY_LeftShift,
	AV_KEY_RightControl,
	AV_KEY_LeftControl,
	AV_KEY_RightAlt,
	AV_KEY_LeftAlt,
	AV_KEYS_COUNT
} av_key_t;

/* Enumeration of AVGL key modifiers */
typedef enum
{
	AV_KEYMOD_NONE        = 0x00,
	AV_KEYMOD_SHIFT_LEFT,
	AV_KEYMOD_SHIFT_RIGHT,
	AV_KEYMOD_CTRL_LEFT,
	AV_KEYMOD_CTRL_RIGHT,
	AV_KEYMOD_ALT_LEFT,
	AV_KEYMOD_ALT_RIGHT
} av_keymod_t;

#ifdef __cplusplus
}
#endif

#endif /* __AV_KEYS_H */
