-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2018,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      group.lua                                          --
-- Description:   Base class for all group views                     --
--                                                                   --
-----------------------------------------------------------------------

local Group = {__NAME = "Group"}
setmetatable(Group, Group)

function Group.__call(self, params)	
	params = params or {}
	assert(type(params) == "table")
	local self = params

	self.__index = function(_, id)
		local v = rawget(Group, id)
		if v then return v end
		v = self.findid(id)
		if v then return v end
		--error(string.format("No %s defined in %s(%s)", id, self.__NAME, rawget(self, "id") or "noname"))
		return nil
	end

	function self.findid(id)
		for i = 1, #self do
			local kid = rawget(self, i)
			if id == rawget(kid, "id") then
				return kid
			else
				local v = kid.findid(id)
				if v then return v end
			end
		end
	end

	function self.class()
		return Group
	end

	return setmetatable(self, self)
end

function Group.isa(class)
	return class == Group
end

return function (name)
	local GroupClass = {__NAME = name}
	function GroupClass.__call(self, params)
		local o = Group(params)
		local gindex = o.__index
		o.__index = function(_, id)
			local v = rawget(GroupClass, id)
			if v then return v end
			return gindex(o, id)
		end
		return setmetatable(o, o)
	end

	function GroupClass.__index(_, id)
		local v = rawget(GroupClass, id)
		if v then return v end
		return function(params)
			group = GroupClass(params)
			group.id = id
			return group
		end
	end
	setmetatable(GroupClass, GroupClass)
	return GroupClass
end
