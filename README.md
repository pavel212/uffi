# uFFI
FFI for Lua, https://habr.com/ru/articles/746658/
#Build
Corresponding build_ .bat/.sh files
# Usage
```Lua
local ffi = require "ffi"
```
## Load
### Single function
```Lua
local msg = ffi("user32", "MessageBoxA")
msg(0, "Text", "Caption", 1)
```
### Global variable
```Lua
local err = ffi("gpib", "user_iberr", 4)
print(err:int())
```
With number as [type](https://github.com/pavel212/uffi#types) (3rd argument) ffi creates and returns [userdata](https://github.com/pavel212/uffi#userdata) of the given size, pointing to library symbol
### List of functions
```Lua
local gl = ffi("opengl32", {"glClearColor", "glClear", "glViewport"})
gl.glViewport(0, 0, 640, 480)
```
### All functions from library
```Lua
for k,v in pairs(ffi("glfw3", "*")) do _G[k] = v end
glfwInit()
```
or 
```Lua
local glfw = {}
for k,v in pairs(ffi("glfw3", "*")) do glfw[k:match("^glfw(.+)") or k] = v end
glfw.Init()
```
### Lazy load
If only library name provided, ffi returns empty table with '__index' metamethod that loads functions at the moment when it is called for the first time (indexed).
```Lua
local std = ffi("msvcrt")
std.puts("Hello msvcrt")
```
#### List of libraries
```Lua
libs = ffi({"msvcrt", "kernel32", "user32"})
libs.MessageBoxA(0, "Text", "Caption", 1)
```
### OpenGL extensions example
Adds wglGetProcAddress to gl = ffi("opengl32") functionality, and removes 'gl' prefix.
```Lua
local gl = {}
gl.GetProcAddress = ffi("opengl32", "wglGetProcAddress")
local mt = getmetatable(gl) or {}
function mt.__index(t, k) 
  t[k] = ffi("opengl32", "gl"..k) or ffi(t.GetProcAddress("gl"..k)) or getmetatable(t).__lib_error(t, "gl"..k)
  return t[k]
end
function mt.__lib_error(t, k) 
  error("Unable to load '"..k.."' from opengl32.dll or wglGetProcAddress") 
end
setmetatable(gl, mt)

gl.ShaderSource(id, 2, {"shder source code", "more shader source code"}, 0)
```
## Types
### Specified
As additional string, one character per argument, first - return value type
```Lua
local cosf = ffi("msvcrt.dll", "cosf", "ff")
print( cosf(math.pi / 3), math.cos(math.pi / 3) )
```
* 'v' - void
* 'i' - integer
* 'f' - float
* 'd' - double
* 'b' - boolean
* 'p' - pointer
* 's' - string
* ~~'t' - table~~
* ~~'l' - use Lua type of argument~~

Structures that are larger than 8 bytes should be passed to Cfunction as pointer(userdata) or string, <= 8 bytes must be packed in integer.
### Automatic
When not specified, use Lua type of argument, but there are no single preision floats in Lua, only double.
```Lua
local cosf = ffi("msvcrt.dll", "cosf")
print( ffi.float( cosf( ffi.float(math.pi / 3) ) ) )
```
* ffi.float(x) - convert function argument to float for Cfunction
* ffi.float(cfunc(x)) - convert float result of Cfunction without specified type to lua_Number
* ffi.double(cfunc(x)) - convert double result of Cfunction without specified type to lua_Number
* ffi.int(cfunc(x)[, len]) - convert integer result of Cfunction without specified type to lua_Integer (default, omit)
* ffi.bool(cfunc(x)[, len]) - convert integer result of Cfunction without specified type to boolean
* ffi.string(cfunc(x)[, len]) - convert integer result (char * pointer) of Cfunction without specified type to string, 'len' bytes or until '\0'
* ffi.pointer(cfunc(x)) - convert integer result of Cfunction without specified type to lightuserdata
## Callback
When Lua function passed to ffi(lua_func, type_string) it is converted to callback and then could be passed as function pointer to C function.
type_string must be present, no automatic type conversion for callbacks!
```Lua
local GLFW = {KEY_ESCAPE = 256, PRESS = 1, RELEASE = 0, TRUE = 1, FALSE = 0}

local function key_callback(window, key, scancode, action, mods)
--  print(key,scancode,action)
  if (key == GLFW.KEY_ESCAPE and action == GLFW.PRESS) then
    glfw.SetWindowShouldClose(window, GLFW.TRUE)
  end
end

--typedef void(* GLFWkeyfun) (GLFWwindow *, int, int, int, int)
key_callback = ffi(key_callback, "vpiiii")

glfw.SetKeyCallback(window, key_callback);
```
## Userdata

* x = ffi.userdata(N) - allocates N bytes, to pass 'x' as pointer to C functions.
* x:type(t) - sets type (function x:int, x:float, ...) for indexing userdata as array with [], not set by default
* x:size(s) - sets size for indexing integer userdata as array with [], not set by default.
* x[pos] - indexing of userdata as array of type which is set by x:type, 0-based.
* x = ffi.userdata("qwerty") - allocates strlen + 1 bytes, and copies string data with '\0'.
* x = ffi.userdata({"first string", "second string"}) - allocates array of char * pointers and then for each allocate and copy corresponding string with '\0', to pass x as char ** to C function.
* x = ffi.userdata(N,K,M) - allocates NxKxM bytes, to pass 'x' as char * to C functions ~~indexing of multidimensional arrays as x[n][k][m]~~
* x = ffi.userdata({N,K,M}) - allocates two dimensional array with 3 elements, N, K, M bytes each, to pass 'x' as char ** pointer to C functions, nested tables for char ****...
* x:int([value][,offset][, len]) - "dereference" userdata with offset*len byte offset and convert to signed integer, len = 1..8, default - length of userdata or 8, x:int(nil, offset, len) to read with offset.
* x:uint([value][, offset][, len]) - "dereference" userdata and convert to unsigned integer, len = 1..8
* x:float([value][, offset]) - "dereference" userdata and convert 4 bytes as float to lua number
* x:double([value][, offset]) - "dereference" userdata and convert 8 bytes as double to lua number
* x:boolean([value][, offset][, len]) - "dereference" userdata and convert len bytes to lua boolean, default 1
* x:string([, value/len][, offset]) - if type(value) =="number" then "dereference" userdata and convert to lua string, 0 or nil for whole string, 
  __tostring=x:string(). if type(value) == "string" then copy string into userdata 
* x:bits([value][, offset][, len]) "dereference" userdata and convert 'len' bits (<=64, default 1) with 'offset' to integer, if integer 'value' provided, write first 'len' bits of it to userdata with 'offset' (in bits, 0-based).
For reading/writing bitfields in structures without byte read-modify-write with unpack/pack.
* x:pack(fmt, v1, v2, ...) uses string.pack to pack binary data into userdata
* ~~x[pos]:pack(fmt, v1, v2, ...) same but with offset~~
* x:unpack(fmt, [pos]) uses string.unpack to unpack binary data from userdata

# [Examples](example/)
