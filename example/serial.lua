local ffi = require ("ffi")
local win = ffi("kernel32.dll")
local M = {}

--bist: 7/8; parity: 0/1/2/3/4 == NO/ODD/EVEN/MARK/SPACE; stop: 0/1/2 == 1/1.5/2
function M:new(port, baudrate, bits, parity, stop)
  local this = {}
  setmetatable(this, {__index = M})
  return port and this:open(port, baudrate, bits, parity, stop) or this 
end

function M:delete()
  self:close()
end

function M:open(port, baudrate, bits, parity, stop)
  baudrate, bits, parity, stop = baudrate or 9600, bits or 8, parity or 0, stop or 1
  self.hCom = win.CreateFile(port, 0xC0000000, 0, 0, 3, 0, 0) -- GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING
  if self.hCom == 0 then return nil end
  if win.SetCommTimeouts(hCom, ("I4"):rep(5):pack(0xFFFFFFFF,0,0,0,0)) == 0 then 
    self:close()
    return nil 
  end
  if win.SetCommState(hCom, ("I4I4I4I2I2I2BBBbbbbbI2"):pack(28, baudrate, 0,   0, 0, 0,   bits, parity, stop,   0, 0, 0, 0, 0, 0)) ~= 0 then
    self:close()
    return nil
  end
  return self
end

function M:close()
  win.CloseHandle(self.hCom)
  self.hCom = nil
end

function M:read(num)
  num = num or self:num()
  local buff = ffi.userdata(num)
  local plength = ffi.userdata(4)
  win.ReadFile(self.hCom, buff, num, plength, 0)
  return buff:string(plength:int())
end

function M:write(str)
  local plength = ffi.userdata(4)
  win.WriteFile(self.hCom, str, #str, plength, 0)
  return plength:int()
end

function M:num()
  local stat = ffi.userdata(12)
  local err = ffi.userdata(4)
  win.ClearCommError(self.hCom, err, stat);
  local s, rx, tx = stat:unpack("I4I4I4")
  return rx, tx
end

function M:clear()
  win.PurgeComm(self.hCom, 0x08) --PURGE_RXCLEAR
end

function M:status()
  local stat = ffi.userdata(4)
  win.GetCommModemStatus(self.hCom, stat)
  return stat:int()
end

function M:rts(value)
  local dcb = ffi.userdata(28)
  win.GetCommState(self.hCom, dcb)
  dcb:bits(69, 2, value and 1 or 0)    --dcb.fRtsControl = RTS_CONTROL_ENABLE
  win.SetCommState(self.hCom, dcb)
end

function M:dtr(value)
  local dcb = ffi.userdata(28)
  win.GetCommState(self.hCom, dcb)
  dcb:bits(77, 2, value and 1 or 0)    --dcb.fDtrControl = DTR_CONTROL_ENABLE
  win.SetCommState(self.hCom, dcb)
end

function M:csr() return self:status() & 0x01 == 0x01 end  --MS_CTS_ON
function M:dsr() return self:status() & 0x02 == 0x02 end  --MS_DSR_ON
function M:ri()  return self:status() & 0x04 == 0x04 end  --MS_RING_ON
function M:dcd() return self:status() & 0x08 == 0x08 end  --MS_RLSD_ON

return setmetatable(M, {__call = M.new, __gc = M.delete})