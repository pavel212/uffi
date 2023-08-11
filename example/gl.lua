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


function gl.ShaderError(shader, err_type)
  local func = {}
  func[GL.COMPILE_STATUS] = {get = gl.GetShaderiv,  info = gl.GetShaderInfoLog}
  func[GL.LINK_STATUS]    = {get = gl.GetProgramiv, info = gl.GetProgramInfoLog}
  func = func[err_type]
  if not func then error ("unknown type: ".. err_type) end
  local pstatus = ffi.userdata(4)
  func.get(shader, err_type, pstatus)
  if pstatus:int() == GL.FALSE then
    func.get(shader, GL.INFO_LOG_LENGTH, pstatus);
    local len = pstatus:int()
    local err = ffi.userdata(len+1)
    len = func.info(shader, len, pstatus, err)
    print(#(err:string()))
    error (err:string())
  end
end

function gl.MakeProgram(vs, fs)
  local pid  = gl.CreateProgram()
  local vsid = gl.CreateShader(GL.VERTEX_SHADER)
  local fsid = gl.CreateShader(GL.FRAGMENT_SHADER)
  gl.ShaderSource(vsid, 1, {vs}, 0)
  gl.ShaderSource(fsid, 1, {fs}, 0)
  gl.CompileShader(vsid)
  local err = gl.ShaderError(vsid, GL.COMPILE_STATUS) if err then return nil, err end
  gl.CompileShader(fsid)
  local err = gl.ShaderError(fsid, GL.COMPILE_STATUS) if err then return nil, err end
  gl.AttachShader(pid, vsid)
  gl.AttachShader(pid, fsid)
  gl.LinkProgram(pid)
  local err = gl.ShaderError(pid, GL.LINK_STATUS) if err then return nil, err end
  gl.UseProgram(pid)
  return pid
end

gl.GL = GL
return gl