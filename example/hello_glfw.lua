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

local vs = [[#version 150
in vec2 p; void main(){ gl_Position = vec4(p.xy, 0.0f, 1.0f); }
]]

local fs = [[#version 150
void main(void){
  vec2 c=vec2(-0.75,0)+gl_FragCoord.xy*vec2(1.33,1.0)*0.003;
  vec2 z=vec2(0.0);
  float h=0.0;
  float m;
  for(int i=0;i<100;i++){
    z=vec2(z.x*z.x-z.y*z.y,2.0*z.x*z.y)+c;
    m=dot(z,z);
    if(m>100000.0) break;
    h+=1.0;
  }
  h=h+1.0-log2(.5*log(m));
  gl_FragColor=vec4(h*0.05);
}
]]

gl.MakeProgram(vs, fs)

while not glfw.WindowShouldClose(window) do
  glfw.GetFramebufferSize(window, pwidth, pheight)  
  gl.Viewport(0, 0, pwidth:int(), pheight:int())    --dereference userdata pointer to integer, same as pwidth:upack("I4")
  gl.Clear(GL.COLOR_BUFFER_BIT)
  gl.Rects(-1,-1,1,1)
  glfw.SwapBuffers(window)
  glfw.PollEvents()
end

glfw.DestroyWindow(window)
glfw.Terminate()
