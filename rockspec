package = "avgl"
version = "0.9.0-0"

source = {
	url = "http://bg.aviq.com/avgl/snapshot/avgl-0.9.0-0.zip"
}

description = {
	summary = "Audio Video Graphics Library",
	detailed = [[
		AVGL is a development library for 2D, Multimedia and GUI
	]],
	license = "Copyright (c) AVIQ Bulgaria Ltd.",
	homepage = "http://bg.aviq.com"
}

dependencies = {
	"a52 >= 0.7.4",
	"avcodec >= 51.71.0",
	"avformat >= 52.22.1",
	"avutil >= 49.10.0",
	"cairo >= 1.8.6",
	"expat >= 2.0.1",
	"faac >= 1.28.0",
	"faad2 >= 2.7.0",
	"freetype >= 2.3.9",
	"lame >= 3.98.0",
	"ogg >= 1.1.3",
	"png >= 1.2.16",
	"sdl >= 1.2.13",
	"xvid >= 1.2.1",
	"vorbis >= 1.1.3",
	"theora >= 1.1.3",
	"xvfb >= 1.3.0",
	"xembed >= 0.1.6",
	"xine >= 0.1.2",
	"vlc >= 1.0.0"
}

external_dependencies = {
}

build = {
	type = "cmake",
	variables = {
	},
	distribution = {
		arch = {
			["linux-x86"] = {
				libraries = {
					"lib/libavgl.so",
					"lib/libavxembed.so",
				},
				executables = {
				},
			},
		},
		etc = {
			"avgl.prefs"
		}
	}
}
