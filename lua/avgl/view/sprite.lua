-----------------------------------------------------------------------
--                                                                   --
-- Copyright (C) 2019,  Alexander Marinov                            --
--                                                                   --
-- Project:       avgl scripting - GUI                               --
-- Filename:      sprite.lua                                         --
-- Description:   Sprite widget                                      --
--                                                                   --
-----------------------------------------------------------------------

local View = require "avgl.view.view"

local Sprite = View("Sprite", {
    classname = "sprite",
    duration = 1000,
    loop = false
})

local function createsurface(self, system)
    if not self.surface then
        self.surface = assert(system:createsurface(self.image))
        if not self.width and not self.height then
            self.width, self.height = surface:getsize()
        end
    end
end

function Sprite:oncreate(window)
    print("Sprite:oncreate")
    createsurface(self, window:system())
    window:setsurface(self.surface)
    assert(window:setsize(self.width, self.height))
    if self.sequence then
        assert(window:setsequence(self.sequence, self.duration, self.loop))
    end
end

function Sprite:getcontentsize()
    createsurface(self, self.__window:system())
    return self.width, self.height
end

return Sprite
