-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2019,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      _hvbox.lua                                         --
-- Description:   Horizontal or vertical box layout                  --
--                                                                   --
-----------------------------------------------------------------------

local Group = require "avgl.view.group"

return function (classname, ishbox)
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
