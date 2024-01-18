local ffi = require ("ffi")
local cosf = ffi("libm.so.6", "cosf", "ff")
print("cosf:", cosf(1.57))

local cos = ffi("libm.so.6", "cos")
print("cos:", cos(1.57))
os.exit()


io.read()
local ffi = require ("ffi")
local lib = ffi({"user32","msvcrt"})
local msg = ffi("user32","MessageBoxA")
local c = ffi("msvcrt", "cos")
msg(0,"C","D",0)
print(c(math.pi/3))
lib.MessageBoxA(0,"A","B",0)
print(lib.cos(math.pi/4))
print(ffi.float(lib.cosf(ffi.float(math.pi/3))))
os.exit()


package.path = package.path ..";.\\example\\?.lua"
local ffi = require ("ffi")
local gl = require ("gl")
os.exit()

package.path = package.path ..";.\\example\\?.lua"
dofile ("example/hello_glfw.lua")
os.exit()

local ffi = require("ffi")
local win = ffi({"user32", "kernel32"})
local hInstance = win.GetModuleHandleA(0)
os.exit()

local ffi = require("ffi")
x=ffi.userdata(256):type("float")
print(x:float())
print(x(0), x(1))
x:pack("ff", 42.0, 43.0)
x[2] = 44
print(x:float())
print(x(0), x(1), x(2))
print(x[0], x[1], x[2])
os.exit()