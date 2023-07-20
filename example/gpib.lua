local ffi = require ("ffi")
local gpib = setmetatable({}, {__index = function(t,k) t[k] = ffi("gpib", "l_ib"..k) return t[k] end})

local M = {}

function M.timeout(t) if t > 0 then return math.floor(math.log(t, math.sqrt(10))+11.5) else return 0 end end

function M:new(dev, pad, sad, timeout, eot, eos) 
  return setmetatable({_dev = gpib.dev(dev or 0, pad or 0, sad or 0, self.timeout(timeout), eot or 0, eos or 0x140A)}, {__index = M}) 
end

function M:delete() gpib.onl(self._dev, 0) end

function M:rd(num)
  local buff = ffi.userdata(num)
  gpib.rd(self._dev, buff, num)
  return buff:string()
end

return setmetatable(M, {__call = M.new, __gc = M.delete, __index = function(t, k) return function(t, ...) gpib.k(self._dev, ...) end end })