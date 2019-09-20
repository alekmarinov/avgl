-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2018,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      drawtext.lua                                       --
-- Description:   text drawing utilities                             --
--                                                                   --
-----------------------------------------------------------------------

-- Note: The text must be UTF8 encoded

local Rect     = require "avgl.rect"
local utf8     = utf8 or require "lua-utf8"

utf8.sub = utf8.sub or function (s, i, j)
    i = utf8.offset(s, i)
    j = utf8.offset(s, j+1) - 1
    return string.sub(s, i, j)
end

--------------------------------------------------------------------------------
-- private functions -----------------------------------------------------------
--------------------------------------------------------------------------------

local function normalizetext(text, textidx, charcount)
	if "table" == type(text) then
		text = table.concat(text)
	else
		text = tostring(text or "")
	end
	local textlen = utf8.len(text)
	charcount = charcount or textlen
	if charcount > 0 then
		textidx = textidx or 0
		return utf8.sub(text, textidx + 1, textidx + charcount)
	end
	return ""
end

local function toutf8(s)
	local codes = {}
	local l = string.len(s)
	for i=1, l do
		table.insert(codes, string.byte(s, i))
	end
	return utf8.char(table.unpack(codes))
end

--------------------------------------------------------------------------------
-- public functions --------------------------------------------------------------
--------------------------------------------------------------------------------

local DrawText = {}

function DrawText.textextents(g,
							fontface, fontslant, fontweight, -- font face description
							fontsize,                        -- font attributes
							text, textidx, charcount         -- text
							)

	text = normalizetext(text, textidx, charcount)
	g:save()
	g:setfontface(fontface, fontslant, fontweight)
	g:setfontsize(fontsize)
	local te = g:gettextextents(text)
	g:restore()
	return text, te
end

function DrawText.textextentsalign(g,
	bbox,                            -- bounding box
	alignx, aligny,                  -- alignment
	fontface, fontslant, fontweight, -- font face description
	fontsize,                        -- font attributes
	text, textidx, charcount         -- text
	)

	local r = Rect(bbox)
	local text, te = DrawText.textextents(g, fontface, fontslant, fontweight, fontsize,
										  text, textidx, charcount)

	local x, y = r.x, r.y
	if "right" == alignx then
		x = x + r.w - te.width
	elseif "center" == alignx then
		if r.w > te.width then
			x = x + (r.w - te.width) / 2
		end
	elseif "left" ~= alignx then
		-- assert(nil, "alignx must be any of [`left', `center', `right'], got `"..alignx.."'")
	end
	if "bottom" == aligny then
		y = y + r.h - te.height
	elseif "center" == aligny then
		y = y + (r.h - te.height) / 2
	elseif "top" ~= aligny then
		-- assert(nil, "aligny must be any of [`top', `center', `bottom'], got `"..aligny.."'")
	end

	return text, Rect(x - te.xbearing, y - te.ybearing, te.width, te.height)
end

function DrawText.wordwrapiterator(g, size, fontface, fontslant, fontweight, fontsize, text)

	local text = text or ""
	-- trim right
	while utf8.match(utf8.sub(text, -1), "%s") do
		text = utf8.sub(text, 1, -2)
	end
	local charcount = utf8.len(text)
	local sidx = 1

	return function()
		local tw = 0
		local count = 1
		local te
		while tw < size and (sidx + count - 1) < charcount do
			_, te = DrawText.textextents(g, fontface, fontslant, fontweight, fontsize,
										          text, sidx - 1, count)
			tw = te.width
			if tw < size then
				count = count + 1
			end
		end
		if tw > 0 then
			local result = utf8.sub(text, sidx, sidx + count - 1)
			-- trim right
			while utf8.match(utf8.sub(result, -1), "%s") do
				result = utf8.sub(result, 1, -2)
			end

			_, te = DrawText.textextents(g, fontface, fontslant, fontweight, fontsize, result)
			tw = te.width
			if tw >= size then
				if utf8.match(result, "%s") then
					while utf8.len(result) > 0 and utf8.match(utf8.sub(result, -1), "%S") do
						result = utf8.sub(result, 1, -2)
						count = count - 1
					end
				else					
					if tw > size then
						result = utf8.sub(result, 1, -2)
						count = count - 1
					end
				end
			end
			if count > 0 then
				sidx = sidx + count
				return result
			end
		end
	end
end

function DrawText.boundbox(g,
	bbox,                                    -- bounding box
	alignx, aligny,                          -- alignment
	fontface, fontslant, fontweight,         -- font face description
	fontsize, fontcolor, fontbordercolor,
	fontbordersize,                          -- font attributes
	text, textidx, charcount,                -- text
	selstart, selend, selcolor, selfontcolor -- selection info
	)

	local textbox
	text, textbox = DrawText.textextentsalign(g, bbox, alignx, aligny, fontface, fontslant,
									          fontweight, fontsize, text, textidx, charcount)

	g:save()

	-- draw text
	g:setfontface(fontface, fontslant, fontweight)
	g:setfontsize(fontsize)
	g:moveto(textbox.x, textbox.y)
	if fontbordercolor then
		g:textpath(text)
		g:setcolorrgba(table.unpack(fontbordercolor))
		g:setlinewidth(fontbordersize)
		g:stroke(true)
		g:setcolorrgba(table.unpack(fontcolor))
		g:fill()
	else
		g:setcolorrgba(table.unpack(fontcolor))
		g:showtext(text)
	end
	
	-- draw selection
	if selstart and selend then
		if selstart > selend then
			selstart, selend = selend, selstart
		end
		charcount = utf8.len(text)
		selstart = selstart - textidx
		selend = selend - textidx
		local selcount = selend - selstart

		if selcount > 0 then
			local selbox1, selbox2
			local selbox = Rect{0, bbox.y, 0, bbox.h}
			if selstart > 0 then
				_, selbox1 = DrawText.textextentsalign(g, bbox, alignx, aligny, fontface, fontslant,
											           fontweight, fontsize, text, 0, selstart)
				selbox.x = selbox1.x + selbox1.w + 0
			else
				selbox.x = bbox.x
			end

			_, selbox2 = DrawText.textextentsalign(g, bbox, alignx, aligny, fontface, fontslant,
										           fontweight, fontsize, text, 0, selstart + selcount)
			selbox.w = selbox2.w + selbox2.x - selbox.x
			g:rectangle(selbox)
			g:setcolorrgba(table.unpack(selcolor))
			g:fill()

			-- draw selected text
			g:moveto(selbox.x, textbox.y)
			if selstart < 0 then
				selcount = selcount + selstart
				selstart = 0
			end
			local seltext, seltextbox = DrawText.textextentsalign(g, selbox, alignx, aligny, fontface, fontslant,
										                          fontweight, fontsize, text, selstart, selcount)
			g:setcolorrgba(table.unpack(selfontcolor))
			g:showtext(seltext)
		end
	end

	g:restore()
	return textbox
end

function DrawText.cursor(g, x, y1, y2, color)
	g:save()
	g:setlinewidth(3) -- FIXME: avoid hardcoded cursor size
	g:setlinecap("ROUND")
	g:setcolorrgba(table.unpack(color))
	g:moveto(x, y1)
	g:lineto(x, y2)
	g:stroke()
	g:restore()
end

function DrawText.boundboxcursor(g,
	bbox,                                      -- bounding box
	alignx, aligny,                            -- alignment
	fontface, fontslant, fontweight,           -- font face description
	fontsize, fontcolor,                       -- font attributes
	text, textidx,                             -- text
	selstart, selend, selcolor, selfontcolor,  -- selection info
	cursorcolor                                -- cursor color
	)

	boundbox(g, bbox, alignx, aligny, fontface, fontslant,
			 fontweight, fontsize, fontcolor, nil, nil, text, textidx, nil,
			 selstart, selend, selcolor, selfontcolor, selfontcolor)

	local _, r = DrawText.textextentsalign(g, bbox, alignx, aligny,
										   fontface, fontslant, fontweight,
									 	   fontsize, text, textidx, selend - textidx)

	cursor(g, 0 + r.x + r.w, bbox.y, bbox.y + bbox.h, cursorcolor)
end

function DrawText.charsinwidth(g,
	fontface, fontslant, fontweight, fontsize, -- font attributes
	text, textidx,                             -- text
	width                                      -- width
	)
	textidx = textidx or 0
	local charscount, currentwidth = 0, 0
	local currenttext = ""
	text = normalizetext(text, textidx)
	local tlen = utf8.len(text)
	while width > currentwidth and charscount <= tlen do
		charscount = charscount + 1
		currenttext = currenttext..utf8.sub(text, charscount, charscount)
		_, currentwidth, _, _, _ = textextents(g,
			fontface, fontslant, fontweight, fontsize, currenttext)
	end
	if charscount > 0 then
		charscount = charscount - 1
	end
	return charscount
end

return DrawText
