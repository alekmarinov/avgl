local Scene = require "avgl.scene"
local Sprite = require "avgl.view.sprite"
local VBox  = require "avgl.layout.vbox"

local sprite1 = Sprite{image="fishes.jpg"}
local sprite2 = Sprite{image="fishes.jpg"}
local scene = Scene(800, 600)

scene:inflate(VBox{sprite1}) -- {sprite1, sprite2})
scene:loop()
