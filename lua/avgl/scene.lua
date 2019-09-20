-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2019,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      scene.lua                                          --
-- Description:   Scene class                                        --
--                                                                   --
-----------------------------------------------------------------------
local avgl = require "lavgl"
local View = require "avgl.view.view"

local Scene = {__NAME = "Scene"}
Scene.__index = Scene
setmetatable(Scene, Scene)

local unpack = unpack or table.unpack

-- constructor
function Scene.__call(class, ...)
    local o = {}
    local args = {...}
    if #args == 1 then
        local view = args[1]
        assert(type(view) == "table")
        assert(view.isa(View))
        assert(view.__window, "A scene window must be inflated")
        o.window = view.__window
    else
        local screen_width, screen_height, options = unpack(args)
        assert(type(screen_width) == "number")
        assert(type(screen_height) == "number")
        assert(type(options) == "nil" or type(options) == "table")
        options = options or {}
        options.width = screen_width
        options.height = screen_height
        options.scale_x = options.scale_x or 1
        options.scale_y = options.scale_y or 1
        o.window = avgl.create(options)
    end
    return setmetatable(o, Scene)
end

function Scene:inflate(view)
	local function collectcontentsizes(g, window, layout, collected)
		print("collectcontentsizes", window, layout)
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
			local sublayout = rawget(layout, i)
			classname = sublayout.classname or "visible"
			print("createwindow: classname = ", classname)
			local subwindow = window:createwindow(classname)
			if sublayout.oncreate then
				sublayout:oncreate(subwindow)
			end
			collectcontentsizes(g, subwindow, sublayout, collected)
		end
		return collected
	end
	local surface = self.window:system():graphics():createsurface(1, 1)
	self.window:system():graphics():begin(surface)
	local windowrects = collectcontentsizes(self.window:system():graphics(), self.window, view)
	self.window:system():graphics():end_()

	for _, winrect in ipairs(windowrects) do
		local window = table.remove(winrect, 1)
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
			for i = 1, #layout do
				local sublayout = rawget(layout, i)
				if sublayout.width == "*" or sublayout.height == "*" then
					local rect = sublayout.__window:getrect()
					if sublayout.width == "*" then
						rect[3] = prect[3]
					end
					if sublayout.height == "*" then
						rect[4] = prect[4]
					end
					sublayout.__window:setrect(rect)
				end
				matchparentsizes(sublayout)
			end
		end
	end

	arrangewindows(view)
	matchparentsizes(view)
end

function Scene:loop(self)
    avgl.loop()
end

return Scene
