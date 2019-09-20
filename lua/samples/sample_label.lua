local Scene = require "avgl.scene"
local Label = require "avgl.view.label"
local VBox  = require "avgl.layout.vbox"

local label1 = Label{text="Hello, World!"}
local label2 = Label{text="What's up?"}
local scene = Scene(800, 600)

scene:inflate(VBox{label1, label2})
scene:loop()
