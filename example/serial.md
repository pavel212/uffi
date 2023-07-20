# Serial port
Old NDP960 Position Display Unit serial port readout example
```Lua
local serial = require("serial")

local port = serial("COM3", 9600, 7, 2, 2)  -- 7bit byte, Even parity, 2 stop bits, instead of standart 8N1

port:rts(true)
port:dtr(true)

port:write("\x02")  --CTRL-B == STX -> request data

local str = ""
while str:sub(-4) ~= string.char(0x0D,0x0A,0x0A,0x0A) do str = str .. port:read() end

local x, y, z = str:gsub(" ", "0"):match("X=([%+%-%.%d]+)[%?]?R%c+Y=([%+%-%.%d]+)[%?]?R%c+Z=([%+%-%.%d]+)[%?]?R%c+")
x, y, z = tonumber(x), tonumber(y), tonumber(z)

print(str)     --response string
print(x, y, z) --parsed coordinates

port:close()

```
