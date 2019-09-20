-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2007,  AVIQ Systems AG                              --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      label.lua                                          --
--                                                                   --
-----------------------------------------------------------------------

local View = require "avgl.view"
local DrawText = require "avgl.drawtext"

local function Label(params)
	local self = View(params)
	function self._view.ondraw(g)
		local text = tostring(self.text or "")

		if self.fillcolor then
			DrawBox.drawroundrect(g, Rect{0, 0, self:getsize()}, self.edgesize or edgesize)
			g:set_color_rgba(unpack(self.fillcolor))
			g:fill()
		end
		self.linescount = 0
		local wrap = self.iswrap or iswrap
		local mline = self.multiline or multiline
		local fface = self.fontface or fontface
		local fslant = self.fontslant or fontslant
		local fweight = self.fontweight or fontweight
		local fsize = self.fontsize or fontsize
		local bbox
		if mline or wrap then
			bbox = {x=0, y=0, w=self:getwidth(), h=self.fontsize}
			for line in ((text or "").."\n"):gmatch("(.-)\n") do
				if wrap then
					for wrapline in DrawText.wordwrapiterator(g, self:getwidth(), fface, fslant, fweight, fsize, line) do
						drawtext(self, g, bbox, wrapline)
						bbox.y = bbox.y + fsize
						self.linescount = self.linescount + 1
					end
				else
					drawtext(self, g, bbox, line)
					bbox.y = bbox.y + fsize
					self.linescount = self.linescount + 1
				end
			end
		else
			bbox = {0, 0, self:getsize()}
			drawtext(self, g, bbox, text)
			self.linescount = 1
		end
	end

end

-- Debug
local print,getmetatable,type = print,getmetatable,type

--------------------------------------------------------------------------------
-- class definition ------------------------------------------------------------
--------------------------------------------------------------------------------
module "lrun.gui.av.label"
oo.class(_M, Visible)

--------------------------------------------------------------------------------
-- local functions -------------------------------------------------------------
--------------------------------------------------------------------------------
local function drawtext(self, g, bbox, text)
	DrawText.boundbox(g, bbox,
					self.alignx or alignx, self.aligny or aligny,
					self.fontface or fontface,
					self.fontslant or fontslant,
					self.fontweight or fontweight,
					self.fontsize or fontsize,
					self.fontcolor or fontcolor,
					self.fontbordercolor or fontbordercolor,
					self.fontbordersize or fontbordersize,
					text)
end

--------------------------------------------------------------------------------
-- default attributes ----------------------------------------------------------
--------------------------------------------------------------------------------
fontbordersize  = 1
fontbordercolor = nil
fontcolor       = {1, 1, 1, 1}
fontface        = "Arial"
fontslant       = "normal"  -- normal | italic | oblique
fontweight      = "normal"  -- normal | bold
fontsize        =  30
alignx          = "left"
aligny          = "center"
iswrap          = false
multiline       = false
edgesize        = 0

--------------------------------------------------------------------------------
-- public methods --------------------------------------------------------------
--------------------------------------------------------------------------------
function paint(self, g)
	local text = tostring(self.text or "")

	if self.fillcolor then
		DrawBox.drawroundrect(g, Rect{0, 0, self:getsize()}, self.edgesize or edgesize)
		g:set_color_rgba(unpack(self.fillcolor))
		g:fill()
	end
	self.linescount = 0
	local wrap = self.iswrap or iswrap
	local mline = self.multiline or multiline
	local fface = self.fontface or fontface
	local fslant = self.fontslant or fontslant
	local fweight = self.fontweight or fontweight
	local fsize = self.fontsize or fontsize
	local bbox
	if mline or wrap then
		bbox = {x=0, y=0, w=self:getwidth(), h=self.fontsize}
		for line in ((text or "").."\n"):gmatch("(.-)\n") do
			if wrap then
				for wrapline in DrawText.wordwrapiterator(g, self:getwidth(), fface, fslant, fweight, fsize, line) do
					drawtext(self, g, bbox, wrapline)
					bbox.y = bbox.y + fsize
					self.linescount = self.linescount + 1
				end
			else
				drawtext(self, g, bbox, line)
				bbox.y = bbox.y + fsize
				self.linescount = self.linescount + 1
			end
		end
	else
		bbox = {0, 0, self:getsize()}
		drawtext(self, g, bbox, text)
		self.linescount = 1
	end
end

function settext(self, text)
	self.text = text or ""
	self:invalidate()
end

function gettext(self)
	return self.text
end

function setfontweight(self, weight)
	if self.fontweight ~= weight then
		self.fontweight = weight
		self:invalidate()
	end
end

function setfontcolor(self, color)
	if not Color.equals(self.fontcolor, color) then
		self.fontcolor = color
		self:invalidate()
	end
end

function setfillcolor(self, color)
	if not self.fillcolor or not color or not Color.equals(self.fillcolor, color) then
		self.fillcolor = color
		self:invalidate()
	end
end

function setfontsize(self, fontsize)
	if self.fontsize ~= fontsize then
		self.fontsize = fontsize
		self:invalidate()
	end
end

function getfontsize(self)
	return self.fontsize or fontsize
end

function getlinecount(self)
	return self.linescount or 0
end
