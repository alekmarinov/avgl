require "lavgl"
local DrawText = require "avgl.drawtext"
local View = require "avgl.view"
local Group = require "avgl.group"
local Rect = require "avgl.rect"

-- Label
Label = View("Label", {
	bgcolor    = {0, 0, 1, 1},
	alignx     = "left",
	aligny     = "center",
	fontface   = "Arial",
	fontslant  = "normal",  -- normal | italic | oblique
	fontweight = "normal",  -- normal | bold
	fontsize   =  30,
	fontcolor  = {1, 1, 1, 1},
	fontbordercolor  = {0, 0, 1, 1},
	fontbordersize  = 2
})
function Label:gettext()
	return self.text
end
function Label:getcontentsize(g)
	local _, te = DrawText.textextents(g, self.fontface, self.fontslant, self.fontweight, self.fontsize, self:gettext())
	return te.width, te.height
end
function Label:ondraw(g)
	local function drawtext(g, bbox, text)
		DrawText.boundbox(g, bbox,
						self.alignx,
						self.aligny,
						self.fontface,
						self.fontslant,
						self.fontweight,
						self.fontsize,
						self.fontcolor,
						self.fontbordercolor,
						self.fontbordersize,
						text)
	end

	g:setcolorrgba(table.unpack(self.bgcolor))
	g:rectangle(Rect(0, 0, self:getsize()))
	g:fill(false)
	drawtext(g, Rect(0, 0, self:getsize()), self:gettext())
end

function createVHBox(classname, ishbox)
	local VHBox = Group(classname)

	function VHBox.getcontentsize(self, g)
		local tw, th = 0, 0
		for i = 1, #self do
			local w, h = self[i]:getcontentsize(g)
			if ishbox then
				w, h = h, w
			end
			if tw < w then
				tw = w
			end
			th = th + h
		end
		if ishbox then
			tw, th = th, tw
		end
		return tw, th
	end

	function VHBox.arrange(self)
		local x, y = 0, 0
		for i = 1, #self do
			local sublayout = rawget(self, i)
			local rect = sublayout.__window:getrect()
			sublayout.__window:setrect(x, y, rect[3], rect[4])
			if not ishbox then
				y = y + rect[4]
			else
				x = x + rect[3]
			end
		end
	end
	function VHBox:ondraw(g)
		g:rectangle(0, 0, self:getsize())
		g:setcolorrgba(0, 1, 0, 1)
		g:stroke(false)
	end
	return VHBox
end
-- VBox
VBox = createVHBox("VBox", false)
HBox = createVHBox("HBox", true)

function createlayout(window, layout)
	local surface = window:system():graphics():createsurface(1, 1)
	window:system():graphics():begin(surface)
	local function collectcontentsizes(g, window, layout, collected)
		layout.__window = window
		layout.getsize = function()
			return window:getsize()
		end
		window.ondraw = function(g)
			layout:ondraw(g)
		end
		local winrect = window:getrect()
		local w, h = layout:getcontentsize(g)
		if not layout.width then
			winrect[3] = w
		end
		if not layout.height then
			winrect[4] = h
		end
		table.insert(winrect, 1, window)
		collected = collected or {}
		table.insert(collected, winrect)
		for i = 1, #layout do
			local subwindow = window:createwindow()
			local sublayout = rawget(layout, i)
			collectcontentsizes(g, subwindow, sublayout, collected)
		end
		return collected
	end
	local windowrects = collectcontentsizes(window:system():graphics(), window, layout)
	window:system():graphics():end_()

	for _, winrect in ipairs(windowrects) do
		local window = table.remove(winrect, 1)
		local oldrect = window:getrect()

		window:setrect(winrect)
	end

	local function arrangewindows(layout)
		if #layout > 0 then
			for i = 1, #layout do
				local sublayout = rawget(layout, i)
				arrangewindows(sublayout)
			end
			layout:arrange()
		end
	end

	local function matchparentsizes(layout)
		if #layout > 0 then
			local prect = layout.__window:getrect()
			print("prect = ", Rect(prect), layout.__NAME)
			for i = 1, #layout do
				local sublayout = rawget(layout, i)
				-- print("***", sublayout.text, sublayout.width, sublayout.height)
				if sublayout.width == "*" or sublayout.height == "*" then
					local rect = sublayout.__window:getrect()
					if sublayout.width == "*" then
						rect[3] = prect[3]
					end
					if sublayout.height == "*" then
						rect[4] = prect[4]
					end
					print(Rect(rect))
					sublayout.__window:setrect(rect)
				end
				matchparentsizes(sublayout)
			end
		end
	end

	arrangewindows(layout)
	matchparentsizes(layout)
end

mainwin = avgl.create{
	width = 800,
	height = 600,
	scale_x = 1,
	scale_y = 1
}

function foreach(t)
	for k, v in pairs(t) do
		if not tonumber(k) then
			for i = 1, #t do
				print(t[i].__NAME, k, v)
				t[i][k] = v
			end
		end
	end
	return table.unpack(t)
end

mylayout = VBox{
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

function dump(rootlayout, ins)
	ins = ins or 0
	print(string.rep(" ", 4*ins), rawget(rootlayout, "id"), rootlayout.__NAME, rootlayout)
	for i = 1, #rootlayout do
		dump(rawget(rootlayout, i), ins+1)
	end
end

dump(mylayout)

createlayout(mainwin, mylayout)

avgl.loop()
