-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2018,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      rect.lua                                           --
-- Description:   rect utilitiy                                      --
--                                                                   --
-----------------------------------------------------------------------

local function Rect(x, y, w, h)
	if type(x) == "table" then
		x, y, w, h = table.unpack(x)
	end
	local self = setmetatable({x, y, w, h}, {
		__index = function(t, k)
			if k == "x" then
				return rawget(t, 1)
			elseif k == "y" then
				return rawget(t, 2)
			elseif k == "w" then
				return rawget(t, 3)
			elseif k == "h" then
				return rawget(t, 4)
			end
			return rawget(t, k)
		end,
		__newindex = function(t, k, v)
			if k == "x" then
				rawset(t, 1, v)
			elseif k == "y" then
				rawset(t, 2, v)
			elseif k == "w" then
				rawset(t, 3, v)
			elseif k == "h" then
				rawset(t, 4, v)
			else
				rawset(t, k, v)
			end
		end,
		__tostring = function(t)
			return string.format("{%d,%d,%d,%d}", table.unpack(t))
		end
	})

	function self.copy()
		return Rect(table.unpack(self))
	end

	function self.setpos(x, y)
		self[1] = x
		self[2] = y
	end

	function self.getpos()
		return self[1], self[2]
	end

	function self.setsize(w, h)
		self[3] = w
		self[4] = h
	end

	function self.getsize()
		return self[3], self[4]
	end

	function self.move(dx, dy)
		self[1] = self[1] + dx
		self[2] = self[2] + dy
		return self
	end

	function self.resize(dx, dy)
		self[3] = self[3] + dx
		self[4] = self[4] + dy
		return self
	end

	function self.shrink(amount)
		self[1] = self[1] + amount
		self[2] = self[2] + amount
		self[3] = self[3] + amount
		self[4] = self[4] + amount
		return self
	end

	function self.scale(amount)
		self[1] = self[1] * amount
		self[2] = self[2] * amount
		self[3] = self[3] * amount
		self[4] = self[4] * amount
		return self
	end

	function self.pointinrect(x0, y0)
		local x, y, w, h = table.unpack(self)
		if x0>=x and x0<x+w and y0>=y and y0<y+h then
			return true
		end
	end

	function self.isrectintersect(rect)
		return
			self.pointinrect(rect[1], rect[2]) or
			self.pointinrect(rect[1] + rect[3], rect[2]) or
			self.pointinrect(rect[1], rect[2] + rect[4]) or
			self.pointinrect(rect[1] + rect.w, rect[2] + rect[4]) or
			rect.pointinrect(self[1], self[2]) or
			rect.pointinrect(self[1] + self[3], self[2]) or
			rect.pointinrect(self[1], self[2] + self[4]) or
			rect.pointinrect(self[1] + self[3], self[2] + self[4])
	end

	function self.intersect(rect2)
		local rect1 = self
		local right1, bottom1, right2, bottom2
		right1  = rect1[1] + rect1[3] - 1
		bottom1 = rect1[2] + rect1[4] - 1
		right2  = rect2[1] + rect2[3] - 1
		bottom2 = rect2[2] + rect2[4] - 1
		if right2 >= rect1[1]  and rect2[1] <= right1 and
			bottom2 >= rect1[2] and rect2[2] <= bottom1 then
			local x, y = {}, {}
			x[0] = rect1[1]
			x[1] = right1
			x[2] = rect2[1]
			x[3] = right2
			y[0] = rect1[2]
			y[1] = bottom1
			y[2] = rect2[2]
			y[3] = bottom2
			for i = 0, 3 do
				for j = 0, 3-i do
					if x[j] > x[j+1] then
						x[j], x[j+1] = x[j+1], x[j]
					end
					if y[j] > y[j+1] then
						y[j], y[j+1] = y[j+1], y[j]
					end
				end
			end
			return Rect(x[1], y[1], x[2] - x[1] + 1, y[2] - y[1] + 1)
		end
	end

	return self
end

return Rect
