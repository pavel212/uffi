local ffi = require("ffi")
local gl = require ("gl")
local glfw = require ("glfw")
local GL = gl.GL
local GLFW = glfw.GLFW

glfw.Init()
local window = glfw.CreateWindow(640, 480, "Simple example", 0, 0)
window:MakeContextCurrent()

local key_callback = function(window, key, scancode, action, mods)
  if (key == GLFW.KEY_ESCAPE and action == GLFW.PRESS) then glfw.SetWindowShouldClose(window, GLFW.TRUE) end
end
key_callback = ffi(key_callback, "vpiiii")
window:SetKeyCallback(key_callback)
  
local vs = gl.shader(GL.VERTEX_SHADER, [[#version 150
in vec2 p; 
void main(void){ gl_Position = vec4(p.xy, 0.0f, 1.0f); }]])

local fs = gl.shader(GL.FRAGMENT_SHADER, [[#version 150
uniform vec2 resolution;
uniform float zoom = 1.0;
uniform vec2 pos = vec2(1.25, 0.05);
void main(void){
  vec2 uv = ((2.0 * gl_FragCoord.xy - resolution) / resolution.y) * zoom - pos;
  int i = 0;
  for (vec2 z = vec2(0.0); (dot(z, z) < 65536.0) && (i < 100); i++) z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + uv;
  gl_FragColor = vec4(vec3(sin(i * 0.05)) * vec3(0.5, 1.0, 1.3), 1.0);
}]])

local prog = gl.program(fs, vs)
local pwidth, pheight  = ffi.userdata(4), ffi.userdata(4)  --allocate 4 byte userdata in lua to pass as pointer to C function

gl.ClearColor(0.3, 0.4, 0.5, 1.0)

while window:WindowShouldClose() == GLFW.FALSE do
  window:GetFramebufferSize(pwidth, pheight)
  local w,h = pwidth:int(), pheight:int()   --dereference userdata pointer to integer, same as pwidth:upack("I4")
  gl.Viewport(0, 0, w, h)    
  prog.resolution = {w, h}          --GLSL uniforms by name with __index/__newindex of prog, type convertion inside
  prog.zoom = math.exp(-5+5*math.cos(os.clock()))
  gl.Clear(GL.COLOR_BUFFER_BIT)
  gl.Rects(-1,-1,1,1)
  window:SwapBuffers()
  glfw.PollEvents()
end

window:DestroyWindow()
glfw.Terminate()