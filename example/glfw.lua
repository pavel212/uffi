local ffi = require("ffi")
local glfw = {}
--for k,v in pairs(ffi("glfw3", "*")) do glfw[k:match("^glfw(.+)")] = v end

setmetatable(glfw, {
  __index = function(t, k) t[k] = ffi("glfw3", "glfw"..k) or getmetatable(t).__lib_error("glfw"..k); return t[k] end,
  __lib_error = function(name) error("Unable to load '"..name.."' from glfw3.dll") end
})

local create_window = glfw.CreateWindow
function glfw.CreateWindow(...) return setmetatable({window = create_window(...)}, {__index = function(t,k) return function(self,...) return glfw[k](self.window,...) end end }) end

--glfw.WindowShouldClose = ffi("glfw3", "glfwWindowShouldClose", "bp")        --specify boolean type, otherwire 0 == true in Lua

local GLFW = setmetatable({}, {__index = function(t,k) error ("missing definition of GLFW_"..k) end})

GLFW.KEY_ESCAPE = 256
GLFW.PRESS = 1
GLFW.RELEASE = 0
GLFW.TRUE = 1
GLFW.FALSE = 0

glfw.GLFW = GLFW

return glfw