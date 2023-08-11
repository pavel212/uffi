local ffi = require("ffi")
local glfw = {}
--for k,v in pairs(ffi("glfw3", "*")) do glfw[k:match("^glfw(.+)")] = v end

glfw.WindowShouldClose = ffi("glfw3", "glfwWindowShouldClose", "bp")        --specify boolean type, otherwire 0 == true in Lua
glfw.SetWindowShouldClose = ffi("glfw3", "glfwSetWindowShouldClose", "vpb")

setmetatable(glfw, {
  __index = function(t, k) print(k); t[k] = ffi("glfw3", "glfw"..k) or getmetatable(t).load_error("glfw"..k); print(type(t[k]), t[k]) return t[k] end,
  load_error = function(name) error("Unable to load '"..name.."' from glfw3.dll") end
})

local GLFW = setmetatable({}, {__index = function(t,k) error ("missing definition of GLFW_"..k) end})

GLFW.KEY_ESCAPE = 256
GLFW.PRESS = 1
GLFW.RELEASE = 0
GLFW.TRUE = 1
GLFW.FLASE = 0

glfw.GLFW = GLFW

return glfw