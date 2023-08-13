local ffi = require ("ffi")
local gl = {}
--for k,v in pairs(ffi("opengl32", "*")) do k = k:match("^gl(.+)") if k then gl[k] = v end end

gl.ClearColor = ffi("opengl32", "glClearColor", "vffff")  --float conversion
gl.GetProcAddress = ffi("opengl32", "wglGetProcAddress", "ps")

local GL = setmetatable({}, {__index = function(t,k) error ("missing definition of GL_"..k) end})

GL.TRUE = 1
GL.FALSE = 0

GL.COLOR_BUFFER_BIT = 0x00004000
GL.VENDOR = 0x1F00
GL.RENDERER = 0x1F01
GL.VERSION = 0x1F02
GL.EXTENSIONS = 0x1F03

GL.FRAGMENT_SHADER = 0x8B30
GL.VERTEX_SHADER = 0x8B31
GL.COMPILE_STATUS = 0x8B81
GL.LINK_STATUS = 0x8B82

GL.INFO_LOG_LENGTH = 0x8B84

setmetatable(gl, {
  __index = function(t, k)
    t[k] = ffi("opengl32", "gl"..k) or ffi(t.GetProcAddress("gl"..k)) or getmetatable(t).__lib_error("gl"..k)
    return t[k]
  end,
  __lib_error = function(name) error("Unable to load '"..name.."' from opengl32.dll or wglGetProcAddress") end
})

local function status(id, status, get, info)
  local pstatus = ffi.userdata(4)
  get(id, status, pstatus)
  if pstatus:int() == GL.FALSE then
    get(id, GL.INFO_LOG_LENGTH, pstatus);
    local len = pstatus:int()
    local err = ffi.userdata(len+1)
    len = info(id, len, pstatus, err)
    return err:string()
  end
end

function gl.CompileStatus(id) return status(id, GL.COMPILE_STATUS, gl.GetShaderiv,  gl.GetShaderInfoLog)  end
function gl.LinkStatus(id)    return status(id, GL.LINK_STATUS,    gl.GetProgramiv, gl.GetProgramInfoLog) end

function gl.MakeProgram(vs, fs)
  local pid  = gl.CreateProgram()
  local vsid = gl.CreateShader(GL.VERTEX_SHADER)
  local fsid = gl.CreateShader(GL.FRAGMENT_SHADER)
  gl.ShaderSource(vsid, 1, {vs}, 0)
  gl.ShaderSource(fsid, 1, {fs}, 0)
  gl.CompileShader(vsid)
  local err = gl.CompileStatus(vsid)
  assert(not err, err)
  gl.CompileShader(fsid)
  local err = gl.CompileStatus(fsid)
  assert(not err, err)
  gl.AttachShader(pid, vsid)
  gl.AttachShader(pid, fsid)
  gl.LinkProgram(pid)
  local err = gl.LinkStatus(pid)
  assert(not err, err)
  gl.UseProgram(pid)
  return pid
end


gl.shader = setmetatable({
  compile = function(self)
    if not self.compiled then 
      gl.CompileShader(self.id);
      local err = gl.CompileStatus(self.id)
      assert(not err, err)
      self.compiled = true
    end
    return self
  end
},{
  __call = function(self, type, ...)
    local this = setmetatable({
      id = gl.CreateShader(type)
    }, {
      __index = gl.shader,
      __call = function(self, ...) 
        self.compiled = false
        gl.ShaderSource(self.id, #{...}, {...}, 0)
        return self
      end
    })
    return this(...)
  end
})

for k,v in pairs({"FRAGMENT", "VERTEX", "COMPUTE", "GEOMETRY"}) do gl.shader[v] = function(self, ...) return self(GL[v.."_SHADER"], ...) end end

gl.program = setmetatable({
  update = function(self)
    for k,shader in pairs(self.shaders) do shader:compile() end
    gl.LinkProgram(self.id)
    local err = gl.LinkStatus(self.id)
    assert(not err, err)
    gl.UseProgram(self.id)
    return self
  end
}, {
  __call = function(self, ...)
    local this = {}
    this.id = gl.CreateProgram()
    this.shaders = {...}
    if type(this.shaders[1] == "string") then this.shaders = {gl.shader(GL.VERTEX_SHADER, this.shaders[1]), gl.shader(GL.FRAGMENT_SHADER, this.shaders[2])} end
    for k,shader in pairs(this.shaders) do gl.AttachShader(this.id, shader.id) end
    setmetatable(this, {__index = gl.program})
    return this:update()
  end
})

--[[
csg = gl.shader(GL.FRAGMENT_SHADER, src)
sdflib = gl.shader(GL.FRAGMENT_SHADER, src)
raymarch = gl.shader(GL.FRAGMENT_SHADER, src)
quad = gl.shader(GL.VERTEX_SHADER, src)

prog = gl.program(sdflib, raymarch, quad)

csg:src("")

prog:update()
]]
gl.GL = GL
return gl