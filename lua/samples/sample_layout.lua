local Scene = require "avgl.scene"
local Label = require "avgl.view.label"
local VBox  = require "avgl.layout.vbox"
local HBox  = require "avgl.layout.hbox"

local label1 = Label{text="Hello, World!"}
local label2 = Label{text="What's up?"}
local scene = Scene(800, 600)

function foreach(t)
	for k, v in pairs(t) do
		if not tonumber(k) then
			for i = 1, #t do
				t[i][k] = v
			end
		end
	end
	return table.unpack(t)
end

samplelayout = VBox{
	width = "*",
	Label{text="Hello AVGL!", width = "*", alignx = "center", bgcolor = {1, 0, 0, 1}},

	HBox{
		VBox{
			foreach{
				Label{text="Row 1, Col1"},
				Label{text="Row 1, Col2"},
				Label{text="Row 1, Col3"},
				Label{text="Row 1, Col4"},
				Label{text="Row 1, Col5"},

				width = "*",
				-- fontbordersize = 3,
				fontbordercolor = {0, 0, 0, 1},
				alignx = "left",
			}
		},
		VBox{
			foreach{
				Label{text="Row 2, Col1"},
				Label{text="Row 2, Col2"},
				Label{text="Row 2, Col3"},
				Label{text="Row 2, Col4"},
				Label{text="Row 2, Col5"},

				width = "*",
				-- fontbordersize = 3,
				fontbordercolor = {0, 0, 0, 1},
				alignx = "left",
			}
		},
		VBox{
			foreach{
				Label{text="Row 3, Col1"},
				Label{text="Row 3, Col2"},
				Label{text="Row 3, Col3"},
				Label{text="Row 3, Col4"},
				Label{text="Row 3, Col5"},

				width = "*",
				-- fontbordersize = 3,
				fontbordercolor = {0, 0, 0, 1},
				alignx = "left",
			}
		}
	}
}

scene:inflate(samplelayout)
scene:loop()
