local gpib = require "gpib"

local hp3458a = {}

function hp3458a:new(addr, boardindex)
  boardindex = boardindex or 0
  this = {};
  this.ib = gpib(boardindex, addr, 0, gpib.timeout(1), 0, 0x140A)
--ibdev (int boardindex, int pad, int sad, int timeout, int eot, int eos)
  this.ib:clr()
  this.addr = addr
  this.status = 0
  this.termStr = "\013\010"
  setmetatable(this, {__index = hp3458a, __gc = hp3458a.delete})  
  return this
end

function hp3458a:delete()
--  self.ib:onl(0)
end

function hp3458a:read(num)
  local s = ""
  while s:byte(-1) ~= 0x0a do s = s .. self.ib:rd(num or 256) end
--  self.status = self.ib:ibsta()
--  self.cntl = self.ib:ibcntl()
--  self.error = self.ib:iberr()
  return s:sub(1, -3)
end

function hp3458a:write(...)
--  print(string.format(...))
  local str = string.format(...) .. (self.termStr)
  self.ib:wrt(str, #str)
--  self.status = self.ib:sta()
  return self.gpib:cntl()
end


function hp3458a:init()
  self:write("ID?")   
  local id = self:read()
  if id ~= "HP3458A" then print ("GPIB @ "..self.addr.." is not a HP3458A voltmeter, ID?="..id) return true end
  self:write("RESET")
  self:write("INBUF ON")
--  self:write("TBUFF ON")  
  self:write("ARANGE OFF")
  self:write("DCV 10")        --10V input range
  self:write("NDIG 8")        --accuracy, would be later overwriten by APER
  self:write("AZERO OFF")
  self:write("DELAY 0")
  self:write("DISP OFF")      --with display tunred on, fast trigger makes error.
  self:write("APER 0.001")    --1ms integrating time
  self:write("TIMER 0.005")    --200Hz freq in timer-trigger mode (for offset meas)
end

function hp3458a:display(str)
  if str then return self:write("DISP MSG,'"..str.."'") else return self:write("DISP OFF") end
end

function hp3458a:trigger(num, period)
  self:write("OFORMAT ASCII")
  self:write("MFORMAT SREAL")
  self:write("TRIG AUTO")
  self:write("TARM HOLD")
  if num > 0 then 
    self:display("EXT TRIGGER")
    self:write("MEM FIFO")
    self:write("NRDGS %d,EXT", num);
    self:write("TARM SGL");
  elseif num < 0 then
    self:display("TIMER TRIGGER")
    if period then self:write("TIMER %G", period) end
    self:write("MEM FIFO")
    self:write("NRDGS %d,TIMER", -num);
    self:write("TARM SGL");
  else
    self:display()
    self:write("MEM OFF")
    self:write("NRDGS 1,AUTO");
    self:write("TARM SGL");
    local d = tonumber(self:read())
    self:display(d)
    return d    
  end
end

function hp3458a:count()
  self:write("MCOUNT?")
  local s = self:read()
  return tonumber(s)
end

function hp3458a:memory(offset)
  self:write("RMEM %d,1,1", offset)
  return tonumber(self:read())
end

function hp3458a:data(num)
  local readNum = 128
  if num == 0 then return tonumber(self:read()) end
  num = num or self:count()
  if num == 0 then return {} end
  local pos = 0
  local size = math.min(num - pos, readNum)
  if size == 0 then return {tonumber(self:read())} end
  local ascii_data = ""
  while size > 0 do
    self:write("RMEM %d,%d,1", pos+1, size)
    local str = self:read(size*18)
    ascii_data = ascii_data .. str .. ","
    pos = pos + size
    size = math.min(num - pos, readNum)
  end
  local data = {}
  for s in ascii_data:gmatch("[%deE%+%-%.]+") do table.insert(data, tonumber(s)) end
  return data
end

function hp3458a:measure()
  local s = self:read()
  if s then return tonumber(s) end
end

setmetatable(hp3458a, {__call = hp3458a.new})

return hp3458a