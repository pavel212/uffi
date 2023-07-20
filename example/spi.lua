local spi = {}

function spi:new(dev)       return setmetatable({mpsse = require("mpsse")(dev or 0), buff = ""}, {__index = spi}) end

function spi:init(clkdiv)
  self.mpsse.dirL = 0x1B -- GPIOL3=0, GPIOL2=0, DRDY=0, RESET=1, CS=1, MISO=0, MOSI=1, CLK=1
  self.mpsse.outL = 0x18 -- reset&cs high
  self.mpsse.dirH = 0x00 
  self.mpsse.outH = 0x00
  self.cfg = 0x00        --shadow copy of CHA & POL & LSB/MSB config byte
  self:polarity(false)
  self:msb(true)
  self.mpsse:write(string.char(0x8A, 0x97, 0x8D, 0x86, (clkdiv or 0) & 0xFF, ((clkdiv or 0) >> 8) & 0xFF, 0x80, self.mpsse.outL, self.mpsse.dirL, 0x82, self.mpsse.outH, self.mpsse.dirH) ) 
end

function spi:push(str)      self.buff = self.buff .. str end
function spi:flush()        self.mpsse:write(self.buff); self.buff = "" end

function spi:write(data)    self:push( string.char(self.cfg | 0x10, (#data-1) & 0xFF, ((#data-1) >> 8) & 0xFF) .. data ) end
function spi:transfer(data) self:push( string.char(self.cfg | 0x30, (#data-1) & 0xFF, ((#data-1) >> 8) & 0xFF) .. data ) end
function spi:read(num)      return self.mpsse:read(num or self.mpsse:status()) end
function spi:wait_low()     self:push( string.char(0x88) ) end
function spi:wait_high()    self:push( string.char(0x89) ) end

function spi:io_set(mask) 
  if (mask & 0x00FF) ~= 0 then self.mpsse.outL = self.mpsse.outL | (mask & 0xFF);       self:push(string.char(0x80, self.mpsse.outL, self.mpsse.dirL)) end
  if (mask & 0xFF00) ~= 0 then self.mpsse.outH = self.mpsse.outH | ((mask>>8) & 0xFF);  self:push(string.char(0x82, self.mpsse.outH, self.mpsse.dirH)) end
end

function spi:io_clr(mask) 
  if (mask & 0x00FF) ~= 0 then self.mpsse.outL = self.mpsse.outL & ~(mask & 0xFF);      self:push(string.char(0x80, self.mpsse.outL, self.mpsse.dirL)) end
  if (mask & 0xFF00) ~= 0 then self.mpsse.outH = self.mpsse.outH & ~((mask>>8) & 0xFF); self:push(string.char(0x82, self.mpsse.outH, self.mpsse.dirH)) end
end

function spi:polarity(x)    
  if x then 
    self.cfg = (self.cfg & ~0x05) | 0x04
    self.mpsse.outL = self.mpsse.outL | 0x01 
  else 
    self.cfg = (self.cfg & ~0x05) | 0x01
    self.mpsse.outL = self.mpsse.outL & ~0x01 
  end 
  self.mpsse:write(string.char(0x80, self.mpsse.outL, self.mpsse.dirL)) 
end

function spi:msb(x) if x then self.cfg = (self.cfg & ~0x08) else self.cfg = (self.cfg | 0x08) end end

setmetatable (spi, {__call = spi.new})

return spi