-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2019,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      label.lua                                          --
-- Description:   Label widget                                       --
--                                                                   --
-----------------------------------------------------------------------

local View = require "avgl.view.view"
local DrawText = require "avgl.view._drawtext"
local Rect = require "avgl.rect"

local Label = View("Label", {
	bgcolor    = {0, 0, 0, 1},
	alignx     = "left",
	aligny     = "center",
	fontface   = "Arial",
	fontslant  = "normal",  -- normal | italic | oblique
	fontweight = "normal",  -- normal | bold
	fontsize   =  30,
	fontcolor  = {1, 1, 1, 1},
	fontbordercolor  = {0, 0, 1, 1},
	fontbordersize  = 0
})

function Label:gettext()
	return self.text
end

function Label:getcontentsize(g)
	local _, te = DrawText.textextents(g, self.fontface, self.fontslant, self.fontweight, self.fontsize, self:gettext())
	return te.width, te.height
end

function Label:ondraw(g)
	local rect = Rect(0, 0, self:getsize())
	g:setcolorrgba(table.unpack(self.bgcolor))
	g:rectangle(rect)
	g:fill(false)
    DrawText.boundbox(g,
		rect,
        self.alignx,
        self.aligny,
        self.fontface,
        self.fontslant,
        self.fontweight,
        self.fontsize,
        self.fontcolor,
        self.fontbordercolor,
        self.fontbordersize,
        self:gettext())
end

return Label
