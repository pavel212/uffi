local ffi = require("ffi")
local lib = ffi("tcadsdll")

local function adsaddr(a, p) return ("BBBBBBW"):pack(a[1], a[2], a[3], a[4], a[5] or 1, a[6] or 1, p) end

function M:new(addr, port, group)
  local this = {addr = addr or {127,0,0,1,1,1}, port = port or 801, group = group or 0x4020}
  setmetatable(this, {__index = M})
  return this:open()
end

function M:address(str)
  local port, addr = {}
  addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], port = str:match("(%d+).(%d+).(%d+).(%d+).(%d+).(%d+):(%d+)")
  if #addr == 6 then self.addr = addr end
  self.port = port or self.port  
end

function M:delete()
  self:close()
end

function M:open()
  lib.AdsPortOpen()
  return self
end

function M:close()
  lib.AdsPortClose()
end

function M:version()
  return (lib.AdsGetDllVersion())
end

function M:read(offset, length, group, addr, port)
  addr = addr or self.addr
  port = port or self.port
  group = group or self.group
  offset = offset or 0
  length = length or 1
  local data = ffi.userdata(length)
  lib.AdsSyncReadReq(adsaddr(addr, port), group, offset, length, data)
  return data:string(length)
end

function M:write(offset, data, group, addr, port)
  addr = addr or self.addr
  port = port or self.port
  group = group or self.group
  offset = offset or 0
  lib.AdsSyncWriteReq(adsaddr(addr, port), group, offset, #data, data)
end

function M:readwrite(offset, data, length, group, addr, port)


