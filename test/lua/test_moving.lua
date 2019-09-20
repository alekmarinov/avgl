
-- require "luarocks.require"
local avgl = require "lavgl"
local DrawText = require "avgl.view._drawtext"
local Rect = require "avgl.rect"
-- local Label = require "avgl.label"


alignx="center"
aligny="center"
fontface="Segoe Script"
fontslant="normal"
fontweight="normal"
fontsize=30
fontcolor={0, 0, 1, 1}
fontbordercolor={1, 1, 1, 1}
fontbordersize=2

local function drawtext(g, bbox, text)
	DrawText.boundbox(g, bbox,
					alignx,
					aligny,
					fontface,
					fontslant,
					fontweight,
					fontsize,
					fontcolor,
					fontbordercolor,
					fontbordersize,
					text)
end

mainwin = avgl.create{
	width = 640,
	height = 480
}

function mainwin.onkeydown(key)
	if key == "escape" then
		os.exit()
	end
end

function sethandlers(window, name)
	window.onmousemove = function (x, y)
		-- print(name..": onmousemove", x, y)
	end
	window.onmousehover = function (x, y)
		print(name..": onmousehover", x, y)
	end
	window.onmouseenter = function (x, y)
		print(name..": onmouseenter", x, y)
	end
	window.onmouseleave = function (x, y)
		print(name..": onmouseleave", x, y)
	end
	window.onmousebuttondown = function (button, x, y)
		print(name..": onmousebuttondown", button, x, y)
	end
	window.onmousebuttonup = function (button, x, y)
		print(name..": onmousebuttonup", button, x, y)
	end
end

child1 = mainwin:createwindow()
child1:setrect(10, 10, 200, 100)
sethandlers(child1, "child1")

child2 = mainwin:createwindow()
child2:setrect(100, 200, 250, 200)
sethandlers(child2, "child2")

-- label1 = Label{
-- 	parent = mainwin,
-- 	x = 250,
-- 	y = 50,
-- 	text = "That's a label"
-- }

-- function label1.onmeasure(g)
-- end

function child1.ondraw(g)
	local x, y = child1:getpos()
	local w, h = child1:getsize()
	local text = string.format("%dx%d at %d,%d", w, h, x, y)
	g:rectangle(0, 0, child1:getsize())
	g:setcolorrgba(1, 1, 0, 0.8)
	g:fill(false)
	g:moveto(0, 20)
	textextents = g:gettextextents(text)
	g:setfontsize(15)
	g:setfontface(fontface, fontslant, fontweight)
	g:textpath(text)
	g:setcolorrgba(1-1, 1-1, 1-0, 1)
	g:stroke(false)
end

function child2.ondraw(g)
	g:rectangle(0, 0, child2:getsize())
	g:setcolorrgba(1, 0, 0, 0.8)
	g:fill(false)
	drawtext(g, Rect(0, 0, child2:getsize()), "Hello, AVGL!")	
end

avgl.loop()

print("quit")
