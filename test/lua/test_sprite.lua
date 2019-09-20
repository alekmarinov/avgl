local avgl = require "lavgl"
screen_width = 640
screen_height = 480

mainwin = avgl.create{
	width = screen_width,
	height = screen_height
}

seq_rocketburner = {
	0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 12, 13, 14, 15, 16, 17, 6, 7, 8, 9, 10, 11
}

rocketburner_width = 256
rocketburner_height = 581
rocketburnerfile = "lua/rocketburner.png"
rocketburner_surface = assert(mainwin:system():createsurface(rocketburnerfile))

sprite = assert(mainwin:createwindow("sprite"))
assert(sprite:setsurface(rocketburner_surface))
assert(sprite:setsize(rocketburner_width, rocketburner_height))
assert(sprite:setsequence(seq_rocketburner, 2000, true))
assert(sprite:setrect(0, 0, rocketburner_width, rocketburner_height))

seq_explosion = {
	0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23
}
explosion_width = 64
explosion_height = 64
explosionfile = "lua/explosion_transparent.png"
explosion_surface = assert(mainwin:system():createsurface(explosionfile))

sprites = {}

function createexplosion(x, y)
	local sprite = mainwin:createwindow("sprite")
	sprite:setsurface(explosion_surface)
	sprite:setsize(explosion_width, explosion_height)
	sprite:setsequence(seq_explosion, 700, true)
	sprite:setrect(x, y, explosion_width, explosion_height)
	sprite:setcurrentframe(math.random(23))
	return sprite
end

createexplosion(screen_width/2, screen_height/2)

function mainwin.onkeydown(key)
	if key == "escape" then
		os.exit()
	elseif key == " " then
		local x, y = math.random(screen_width), math.random(screen_height)
		local sprite = createexplosion(x, y)
		table.insert(sprites, sprite)
	elseif key == "delete" then
		if #sprites > 0 then
			sprites[#sprites]:detach()
			table.remove(sprites, #sprites)
		end 
	end
end

avgl.loop()

print("quit")
