local ffi = require ("ffi")
local gl = {}
--for k,v in pairs(ffi("opengl32", "*")) do k = k:match("^gl(.+)") if k then gl[k] = v end end

gl.ClearColor = ffi("opengl32", "glClearColor", "vffff")  --float conversion
gl.GetProcAddress = ffi("opengl32", "wglGetProcAddress", "ps")

local GL = setmetatable({}, {__index = function(t,k) error ("missing definition of GL_"..k) end})

GL.COLOR_BUFFER_BIT = 0x00004000
GL.NUM_EXTENSIONS = 0x821D
GL.VENDOR = 0x1F00
GL.RENDERER = 0x1F01
GL.VERSION = 0x1F02
GL.EXTENSIONS = 0x1F03

setmetatable(gl, {
  __index = function(t, k)
    t[k] = ffi("opengl32", "gl"..k) or ffi(t.GetProcAddress("gl"..k)) or getmetatable(t).load_error("gl"..k)
    return t[k]
  end,
  load_error = function(name) error("Unable to load '"..name.."' from opengl32.dll or wglGetProcAddress") end
})

gl.GL = GL
return gl