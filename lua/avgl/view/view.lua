-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2018,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      view.lua                                           --
-- Description:   Base class for all views                           --
--                                                                   --
-----------------------------------------------------------------------

local View = {__NAME = "View"}
setmetatable(View, View)

function View.__call(self, params)
	if type(params) == "string" then
		params = {text = params}
	end
	params = params or {}
	assert(type(params) == "table")
	local self = params

	function self.findid(id)
		return nil -- View have no kids
	end

	function self.class()
		return View
	end

	return self
end

function View.isa(class)
	return class == View
end

-- View constructor
return function (name, ViewClass)
	ViewClass = ViewClass or {}
	ViewClass.__NAME = name
	function ViewClass.__call(_, params)
		local self = View(params)
		self.__index = function(_, id)
			local v = rawget(ViewClass, id)
			if v then return v end
		end
		function self.class()
			return ViewClass
		end
		return setmetatable(self, self)
	end

	function ViewClass.__index(self, id)
		local v = rawget(ViewClass, id)
		if v then return v end
		return function(params)
			local view = ViewClass(params)
			view.id = id
			return view
		end
	end

	function ViewClass.isa(class)
		if class == ViewClass then
			return true
		end
		return View.isa(class)
	end

	setmetatable(ViewClass, ViewClass)
	return ViewClass
end
