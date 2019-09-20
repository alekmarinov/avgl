
-- require "luarocks.require"
require "lavgl"
local DrawText = require "avgl.drawtext"
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

child1 = mainwin:createwindow()
child1:setrect(10, 10, 200, 100)
child2 = mainwin:createwindow()
child2:setrect(100, 200, 250, 200)
-- label1 = Label{
-- 	parent = mainwin,
-- 	x = 250,
-- 	y = 50,
-- 	text = "That's a label"
-- }

-- function label1.onmeasure(g)
-- end

function child1.ondraw(g)
	-- local x, y = child1:getpos()
	-- local text = string.format("%dx%d at %d,%d", x, y, 100, 100)
	-- g:rectangle(0, 0, child1:getsize())
	-- g:setcolorrgba(1, 1, 0, 0.8)
	-- g:fill(false)
	-- g:moveto(0, 20)
	-- textextents = g:gettextextents(text)
	-- g:setfontsize(30)
	-- g:setfontface(fontface, fontslant, fontweight)
	-- g:textpath(text)
	-- g:setcolorrgba(1-1, 1-1, 1-0, 1)
	-- g:stroke(false)
end

function child2.ondraw(g)
	g:rectangle(0, 0, child2:getsize())
	g:setcolorrgba(1, 0, 0, 0.8)
	g:fill(false)
	-- drawtext(g, Rect(0, 0, child2:getsize()), "Hello, AVGL!")	
end

avgl.loop()

print("quit")
