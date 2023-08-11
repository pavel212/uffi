local ffi = require("ffi")

local gl = require ("gl")
local GL = gl.GL

local glfw = require ("glfw")
local GLFW = glfw.GLFW

glfw.Init()

local window = glfw.CreateWindow(640, 480, "Simple example", 0, 0)
glfw.MakeContextCurrent(window);

local pwidth  = ffi.userdata(4)    --allocate 4 byte userdata in lua to pass as pointer to C function
local pheight = ffi.userdata(("I4"):pack(480)) --same 4 bytes but initialized to 480 with packed string.

local key_callback =  function(window, key, scancode, action, mods)
  print(window, pwidth:int(), pheight:int(), key, GLFW.KEY_ESCAPE, scancode, action, GLFW.PRESS, mods)
  if (key == GLFW.KEY_ESCAPE and action == GLFW.PRESS) then glfw.SetWindowShouldClose(window, GLFW.TRUE) end
end

key_callback = ffi(key_callback, "vpiiii")

glfw.SetKeyCallback(window, key_callback);

gl.ClearColor(0.3, 0.4, 0.5, 1.0)

while not glfw.WindowShouldClose(window) do
  glfw.GetFramebufferSize(window, pwidth, pheight)  
  gl.Viewport(0, 0, pwidth:int(), pheight:int())    --dereference userdata pointer to integer, same as pwidth:upack("I4")
  gl.Clear(GL.COLOR_BUFFER_BIT)
  glfw.SwapBuffers(window)
  glfw.PollEvents()
end

glfw.DestroyWindow(window)
glfw.Terminate()

