require "lavgl"

mainwin = avgl.create{
	width = 640,
	height = 480
}

child1 = mainwin:createwindow()
child1:setrect(10, 10, 200, 100)
function child1.ondraw(g)
	-- g:rectangle(0, 0, child1:getsize())
	-- g:setcolorrgba(1, 0, 0, 0.8)
	-- g:fill(false)
	-- -- drawtext(g, Rect(0, 0, child2:getsize()), "Hello, AVGL!")	
end

avgl.loop()
