local Scene = require "avgl.scene"
local Sprite = require "avgl.view.sprite"
local VBox  = require "avgl.layout.vbox"

local sprite = Sprite{image="fishes.jpg"}
local scene = Scene(800, 600)
scene:inflate(VBox{sprite})
scene:loop()
