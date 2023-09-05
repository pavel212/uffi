package.path = package.path ..";.\\example\\?.lua"

local ffi = require("ffi")
local win = ffi({"user32", "kernel32"})
local hInstance = win.GetModuleHandleA(0)

os.exit()

dofile ("example/hello_glfw.lua")

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


