local ffi = require ("ffi")
local ft = setmetatable({}, {__index = function(t,k) t[k] = ffi("lftd2xx", "FT_"..k) return t[k] end})

local M = {}

function M:new(dev)
  local handle = ft.Open(dev or 0);
  ft.SetUSBParameters(handle, 65536, 65535);	--Set USB request transfer size
  ft.SetChars(handle, 0, 0, 0, 0);	        --Disable event and error characters
  ft.SetTimeouts(handle, 0, 0);		        --Sets the read and write timeouts in milliseconds for the FT2232H
  ft.SetLatencyTimer(handle, 16);		--Set the latency timer
  ft.SetBitMode(handle, 0x0, 0x00); 		--Reset controller
  ft.SetBitMode(handle, 0x0, 0x02);	 	--Enable MPSSE mode
  ft.SetDivisor(handle, 0);	 	        --div 1
  return setmetatable({handle = handle}, {__index = M})
end

function M:delete() 
  ft.Close(self.handle) 
end

function M:write(str) 
  local len = ffi.userdata(4)
  local status = ft.Write(self.handle, str, #str, len)
  return len:int(), status
end

function M:read(num) 
  num = num or self:status()
  local buff = ffi.userdata(num)
  local len = ffi.userdata(4)
  local status = ft.Read(self.handle, buff, num, len)
  return buff:string(), status
end

function M:status() 
  local RxBytes, TxBytes, Event = ffi.userdata(4), ffi.userdata(4), ffi.userdata(4)
  local status = ft.GetStatus(self.handle, RxBytes, TxBytes, Event)
  return RxBytes:int(), TxBytes:int(), Event:int(), status
end

setmetatable(M, {__call = M.new, __gc = M.delete})

return M
