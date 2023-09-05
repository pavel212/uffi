local ffi = require ("ffi")
local gl = {}
--for k,v in pairs(ffi("opengl32", "*")) do k = k:match("^gl(.+)") if k then gl[k] = v end end

gl.ClearColor = ffi("opengl32", "glClearColor", "vffff")  --float conversion
gl.GetProcAddress = ffi("opengl32", "wglGetProcAddress", "ps")

setmetatable(gl, {
  __index = function(t, k)
    t[k] = ffi("opengl32", "gl"..k) or ffi(t.GetProcAddress("gl"..k)) or getmetatable(t).__lib_error("gl"..k)
    return t[k]
  end,
  __lib_error = function(name) error("Unable to load '"..name.."' from opengl32.dll or wglGetProcAddress") end
})

local GL = setmetatable({}, {__index = function(t,k)
  local path = "D:\\soft\\tcc\\include\\GL\\"
  for s in io.lines(path.."gl.h") do if s:match("GL_"..k) then print(s) end end
  for s in io.lines(path.."glext.h") do if s:match("GL_"..k) then print(s) end end
  error ("missing definition of GL_"..k) 
end})

GL.TRUE = 1
GL.FALSE = 0

GL.COLOR_BUFFER_BIT = 0x00004000
GL.VENDOR = 0x1F00
GL.RENDERER = 0x1F01
GL.VERSION = 0x1F02
GL.EXTENSIONS = 0x1F03

GL.COMPUTE_SHADER = 0x91B9
GL.VERTEX_SHADER = 0x8B31
GL.TESS_EVALUATION_SHADER = 0x8E87
GL.TESS_CONTROL_SHADER = 0x8E88
GL.GEOMETRY_SHADER = 0x8DD9
GL.FRAGMENT_SHADER = 0x8B30

GL.TRANSFORM_FEEDBACK = 0x8E22

GL.COMPILE_STATUS = 0x8B81
GL.LINK_STATUS = 0x8B82
GL.INFO_LOG_LENGTH = 0x8B84
GL.ATTACHED_SHADERS = 0x8B85
GL.ACTIVE_UNIFORMS = 0x8B86
GL.ACTIVE_UNIFORM_MAX_LENGTH = 0x8B87
GL.SHADER_SOURCE_LENGTH = 0x8B88
GL.CURRENT_PROGRAM = 0x8B8D

GL.FLOAT = 0x1406
GL.FLOAT_VEC2 = 0x8B50
GL.FLOAT_VEC3 = 0x8B51
GL.FLOAT_VEC4 = 0x8B52
GL.FLOAT_MAT2 = 0x8B5A
GL.FLOAT_MAT3 = 0x8B5B
GL.FLOAT_MAT4 = 0x8B5C

GL.DOUBLE = 0x140A                                                                                                                                                                                                                     
GL.DOUBLE_VEC2 = 0x8FFC
GL.DOUBLE_VEC3 = 0x8FFD
GL.DOUBLE_VEC4 = 0x8FFE
GL.DOUBLE_MAT2 = 0x8F46
GL.DOUBLE_MAT3 = 0x8F47
GL.DOUBLE_MAT4 = 0x8F48

GL.INT = 0x1404
GL.INT_VEC2 = 0x8B53
GL.INT_VEC3 = 0x8B54
GL.INT_VEC4 = 0x8B55

GL.UNSIGNED_INT = 0x1405
GL.UNSIGNED_INT_VEC2 = 0x8DC6
GL.UNSIGNED_INT_VEC3 = 0x8DC7
GL.UNSIGNED_INT_VEC4 = 0x8DC8

GL.BOOL = 0x8B56
GL.BOOL_VEC2 = 0x8B57
GL.BOOL_VEC3 = 0x8B58
GL.BOOL_VEC4 = 0x8B59

local function get_shader_int(id, name)
  local pint = ffi.userdata(4)
  gl.GetShaderiv(id, name, pint)
  return pint:int()
end

local function get_program_int(id, name)
  local pint = ffi.userdata(4)
  gl.GetProgramiv(id, name, pint)
  return pint:int()
end

local function get_int(name)
  local pint = ffi.userdata(4)
  gl.GetIntegerv(name, pint)
  return pint:int()
end

local function compile_status(self) 
  if get_shader_int(self.id, GL.COMPILE_STATUS) == GL.FALSE then
    local len = get_shader_int(self.id, GL.INFO_LOG_LENGTH)
    local info = ffi.userdata(len)
    gl.GetShaderInfoLog(self.id, len, pint, info)
    return info:string(len)
  end
end

local function link_status(self) 
  if get_program_int(self.id, GL.LINK_STATUS) == GL.FALSE then
    local len = get_program_int(self.id, GL.INFO_LOG_LENGTH)
    local info = ffi.userdata(len)
    gl.GetProgramInfoLog(id, len, pint, info)
    return info:string(len)
  end
end

local function uniforms(id)
  local num = get_program_int(id, GL.ACTIVE_UNIFORMS)
  local buff_size = get_program_int(id, GL.ACTIVE_UNIFORM_MAX_LENGTH)
  local length, type, size, name = ffi.userdata(4), ffi.userdata(4), ffi.userdata(4), ffi.userdata(buff_size)
  local types, sizes, locations = {}, {}, {}
  for i = 1, num do
    gl.GetActiveUniform(id, i-1, buff_size, length, size, type, name)
    local n  = name:string(length:int())
    types[n] = type:int()
    sizes[n] = size:int()
    locations[n] = gl.GetUniformLocation(id, n) -- i-1?
  end
  return types, sizes, locations
end

local uniform_set  = { [GL.FLOAT] = "1fv", [GL.FLOAT_VEC2] = "2fv", [GL.FLOAT_VEC3] = "3fv", [GL.FLOAT_VEC4] = "4fv", [GL.DOUBLE] = "1dv", [GL.DOUBLE_VEC2] = "2dv", [GL.DOUBLE_VEC3] = "3dv", [GL.DOUBLE_VEC4] = "4dv", [GL.INT] = "1iv", [GL.INT_VEC2] = "2iv", [GL.INT_VEC3] = "3iv", [GL.INT_VEC4] = "4iv", [GL.UNSIGNED_INT] = "1uiv", [GL.UNSIGNED_INT_VEC2] = "2uiv", [GL.UNSIGNED_INT_VEC3] = "3uiv", [GL.UNSIGNED_INT_VEC4] = "4uiv", [GL.BOOL] = "1iv", [GL.BOOL_VEC2] = "2iv", [GL.BOOL_VEC3] = "3iv", [GL.BOOL_VEC4] = "4iv", [GL.FLOAT_MAT2] = "Matrix2fv", [GL.FLOAT_MAT3] = "Matrix3fv", [GL.FLOAT_MAT4] = "Matrix4fv", [GL.DOUBLE_MAT2] = "Matrix2dv", [GL.DOUBLE_MAT3] = "Matrix3dv", [GL.DOUBLE_MAT4] = "Matrix4dv"}
local uniform_get  = {}
local uniform_num  = {}
local uniform_size = {}
local uniform_pack = {}

for k,v in pairs(uniform_set) do
  local t = v:gsub("Matrix",""):sub(2,2)
  uniform_get [k] = t.."v"
  uniform_size[k] = ({f = 4, d = 8, i = 4, u = 4, b = 4})[t]
  uniform_pack[k] = ({f = "f", d = "d", i = "i4", u = "I4", b = "i4"})[t]
  uniform_num [k] = tonumber(v:gsub("Matrix",""):sub(1,1))
  if v:match("Matrix") then uniform_num[k] = uniform_num[k] * uniform_num[k] end
end

gl.shader = function(type, ...)
  return setmetatable({id = gl.CreateShader(type), status = compile_status}, {
    __call = function(self, ...) 
      local src = {...}
      if #src > 0 then 
        gl.ShaderSource(self.id, #src, src, 0)
        gl.CompileShader(self.id)
        local err = self:status()
        assert(not err, err)
      end
      return self
    end,
    __gc = function(self) gl.DeleteShader(self.id) end
  })(...)  --create, setmetatable, call with gl.shader arguments and return result
end

gl.compute_shader     = function(...) return gl.shader(GL.COMPUTE_SHADER,     ...) end
gl.vertex_shader      = function(...) return gl.shader(GL.VERTEX_SHADER,      ...) end
gl.tesselation_shader = function(...) return gl.shader(GL.TESSELATION_SHADER, ...) end
gl.geometry_shader    = function(...) return gl.shader(GL.GEOMETRY_SHADER,    ...) end
gl.fragment_shader    = function(...) return gl.shader(GL.FRAGMENT_SHADER,    ...) end

gl.program = function(...)  
  return setmetatable({id = gl.CreateProgram(), status = link_status}, {
    __call = function(self, ...)
      local shaders = {...}
      if #shaders > 0 then
        local num = get_program_int(self.id, GL.ATTACHED_SHADERS)
        if num > 0 then
          local pshaders = userdata(4*num)
          gl.GetAttachedShaders(self.id, num, 0, pshaders)
          for i = 0, num-1 do gl.DetachShader(self.id, pshaders:int(nil, i)) end
        end
        if type(shaders[1]) == "string" then shaders = {gl.shader(GL.VERTEX_SHADER, shaders[1]), gl.shader(GL.FRAGMENT_SHADER, shaders[2])} end
        for k, shader in pairs(shaders) do gl.AttachShader(self.id, shader.id) end
      end
      
      if get_program_int(self.id, GL.ATTACHED_SHADERS) > 0 then
        gl.LinkProgram(self.id)
        local err = self:status()
        assert(not err, err)
        gl.UseProgram(self.id)
        local types, sizes, locations = uniforms(self.id)

        getmetatable(self).__index = function(self,k)
          local t = types[k]
          local num = uniform_num[t]
          local size = uniform_size[t]
          local param = ffi.userdata(num * size * sizes[k])
          gl["GetUniform"..uniform_get[t]](self.id, locations[k], param)
          local r = {param:unpack(uniform_pack[t]:rep(num))}
          return (#r > 1) and r or r[1]
        end

        getmetatable(self).__newindex = function(self,k,v)
          assert(get_int(GL.CURRENT_PROGRAM) == self.id, "Setting uniform of program ("..self.id.."), while GL_CURRENT_PROGRAM = "..get_int(GL.CURRENT_PROGRAM))
          local t = types[k]
          local num = uniform_num[t]
          if type(v) ~= "table" then v = {v} end
          local param = uniform_pack[t]:rep(num * sizes[k]):pack(table.unpack(v))
          if uniform_set[t]:match("Matrix") then gl["Uniform"..uniform_set[t]](locations[k], sizes[k], 0, param)
          else gl["Uniform"..uniform_set[t]](locations[k], sizes[k], param) end
        end
      end
      return self
    end,
    __gc = function(self) gl.DeleteProgram(self.id) end
  })(...)  --create table {id = id}, setmetatable, call with gl.program arguments and return result
end

gl.GL = GL
return gl
