local CMD = { NOP = 0x00, RESET = 0x06, START1 = 0x08, STOP1 = 0x0A, START2 = 0x0C, STOP2 = 0x0E, RDATA1 = 0x12, RDATA2 = 0x14, SYOCAL1 = 0x16, SYGCAL1 = 0x17, SFOCAL1 = 0x19, SYOCAL2 = 0x1B, SYGCAL2 = 0x1C, SFOCAL2 = 0x1E, RREG = 0x20, WREG = 0x40}

local REG = { ID = 0x00, POWER = 0x01, INTERFACE = 0x02, MODE0 = 0x03, MODE1 = 0x04, MODE2 = 0x05, INPMUX = 0x06, OFCAL0 = 0x07, OFCAL1 = 0x08, OFCAL2 = 0x09, FSCAL0 = 0x0A, FSCAL1 = 0x0B, FSCAL2 = 0x0C, IDACMUX = 0x0D, IDACMAG = 0x0E, REFMUX = 0x0F, TDACP = 0x10, TDACN = 0x11, GPIOCON = 0x12, GPIODIR = 0x13, GPIODAT = 0x14, ADC2CFG = 0x15, ADC2MUX = 0x16, ADC2OFC0 = 0x17, ADC2OFC1 = 0x18, ADC2FSC0 = 0x19, ADC2FSC1 = 0x1A}

local MASK = {
  STATUS = {ADC2 = 0x80, ADC1 = 0x40, EXTCLK = 0x20, REF_ALM = 0x10, PGAL_ALM = 0x08, PGAH_ALM = 0x04, PGAD_ALM = 0x02, RESET = 0x01},
  POWER = {RESET = 0x10, VBIAS = 0x02, INTREF = 0x01},
  INTERFACE = {TIMEOUT = 0x08, STATUS = 0x04, CRC_DISABLE = 0x00, CRC_SUM = 0x01, CRC_CRC8 = 0x02},
  MODE0 = {REFREV = 0x80, RUNMODE = 0x40, CHOP_INPUT = 0x20, CHOP_IDAC = 0x10, DELAY_OFF = 0x00, DELAY_9us = 0x01, DELAY_17us = 0x02, DELAY_35us = 0x03, DELAY_69us = 0x04, DELAY_139us = 0x05, DELAY_278us = 0x06, DELAY_556us = 0x07, DELAY_1111us = 0x08, DELAY_2222us = 0x09, DELAY_4444us = 0x0A, DELAY_8888us = 0x0B},
  MODE1 = {FILTER = 0xE0, FILTER_SINC1 = 0x00, FILTER_SINC2 = 0x20, FILTER_SINC3 = 0x40, FILTER_SINC4 = 0x60, FILTER_FIR = 0x80, SBADC = 0x10, SBPOL = 0x08, SBMAG_OFF = 0x00, SBMAG_500nA = 0x01, SBMAG_2uA = 0x02, SBMAG_10uA = 0x03, SBMAG_50uA = 0x04, SBMAG_200uA = 0x05, SBMAG_10MOhm = 0x06},
  MODE2 = {BYPASS_PGA = 0x80, GAIN = 0x70, GAIN_1 = 0x00, GAIN_2 = 0x10, GAIN_4 = 0x20, GAIN_8 = 0x30, GAIN_16 = 0x40, GAIN_32 = 0x50, DR = 0x0F, DR_2SPS = 0x00, DR_5SPS = 0x01, DR_10SPS = 0x02, DR_17SPS = 0x03, DR_20SPS = 0x04, DR_50SPS = 0x05, DR_60SPS = 0x06, DR_100SPS = 0x07, DR_400SPS = 0x08, DR_1200SPS = 0x09, DR_2400SPS = 0x0A, DR_4800SPS = 0x0B, DR_7200SPS = 0x0C, DR_14400SPS = 0x0D, DR_19200SPS = 0x0E, DR_38400SPS = 0x0F},
  INPMUX = {MUXP_AIN0 = 0x00, MUXP_AIN1 = 0x10, MUXP_AIN2 = 0x20, MUXP_AIN3 = 0x30, MUXP_AIN4 = 0x40, MUXP_AIN5 = 0x50, MUXP_AIN6 = 0x60, MUXP_AIN7 = 0x70, MUXP_AIN8 = 0x80, MUXP_AIN9 = 0x90, MUXP_AINCOM = 0xA0, MUXP_TEMP = 0xB0, MUXP_AVDD = 0xC0, MUXP_DVDD = 0xD0, MUXP_TDACP = 0xE0, MUXP_OPEN = 0xF0, MUXN_AIN0 = 0x00, MUXN_AIN1 = 0x01, MUXN_AIN2 = 0x02, MUXN_AIN3 = 0x03, MUXN_AIN4 = 0x04, MUXN_AIN5 = 0x05, MUXN_AIN6 = 0x06, MUXN_AIN7 = 0x07, MUXN_AIN8 = 0x08, MUXN_AIN9 = 0x09, MUXN_AINCOM = 0x0A, MUXN_TEMP = 0x0B, MUXN_AVDD = 0x0C, MUXN_DVDD = 0x0D, MUXN_TDACN = 0x0E, MUXN_OPEN = 0x0F},
  IDACMUX = {MUX2_AIN0 = 0x00, MUX2_AIN1 = 0x10, MUX2_AIN2 = 0x20, MUX2_AIN3 = 0x30, MUX2_AIN4 = 0x40, MUX2_AIN5 = 0x50, MUX2_AIN6 = 0x60, MUX2_AIN7 = 0x70, MUX2_AIN8 = 0x80, MUX2_AIN9 = 0x90, MUX2_AINCOM = 0xA0, MUX2_OFF = 0xB0, MUX1_AIN0 = 0x00, MUX1_AIN1 = 0x01, MUX1_AIN2 = 0x02, MUX1_AIN3 = 0x03, MUX1_AIN4 = 0x04, MUX1_AIN5 = 0x05, MUX1_AIN6 = 0x06, MUX1_AIN7 = 0x07, MUX1_AIN8 = 0x08, MUX1_AIN9 = 0x09, MUX1_AINCOM = 0x0A, MUX1_OFF = 0x0B},
  IDACMAG = {MAG2_OFF = 0x00, MAG2_50uA = 0x10, MAG2_100uA = 0x20, MAG2_250uA = 0x30, MAG2_500uA = 0x40, MAG2_7500uA = 0x50, MAG2_1000uA = 0x60 ,MAG2_1500uA = 0x70 ,MAG2_2000uA = 0x80 ,MAG2_2500uA = 0x90 ,MAG2_3000uA = 0xA0 ,MAG1_OFF = 0x00 ,MAG1_50uA = 0x01 ,MAG1_100uA = 0x02 ,MAG1_250uA = 0x03 ,MAG1_500uA = 0x04 ,MAG1_7500uA = 0x05 ,MAG1_1000uA = 0x06 ,MAG1_1500uA = 0x07 ,MAG1_2000uA = 0x08 ,MAG1_2500uA = 0x09 ,MAG1_3000uA = 0x0A},
  REFMUX = {RMUXP_INT = 0x00, RMUXP_AIN0 = 0x08, RMUXP_AIN2 = 0x10, RMUXP_AIN4 = 0x18, RMUXP_AVDD = 0x20, RMUXN_INT = 0x00, RMUXN_AIN1 = 0x01, RMUXN_AIN3 = 0x02, RMUXN_AIN5 = 0x03, RMUXN_AVDD = 0x04},
  TDACP = {OUTP = 0x80},
  TDACN = {OUTN = 0x80},
  ADC2CFG = {DR2_10SPS = 0x00, DR2_100SPS = 0x40, DR2_400SPS = 0x80, DR2_800SPS = 0xC0, REF2_INT = 0x00, REF2_AIN01 = 0x08, REF2_AIN23 = 0x10, REF2_AIN45 = 0x18, REF2_AVDD = 0x20, GAIN_1 = 0x00, GAIN_2 = 0x01, GAIN_4 = 0x02, GAIN_8 = 0x03, GAIN_16 = 0x04 ,GAIN_32 = 0x05, GAIN_64 = 0x06, GAIN_128 = 0x07},
  ADC2MUX = {MUXP2_AIN0 = 0x00, MUXP2_AIN1 = 0x10, MUXP2_AIN2 = 0x20, MUXP2_AIN3 = 0x30, MUXP2_AIN4 = 0x40, MUXP2_AIN5 = 0x50, MUXP2_AIN6 = 0x60, MUXP2_AIN7 = 0x70, MUXP2_AIN8 = 0x80, MUXP2_AIN9 = 0x90, MUXP2_AINCOM = 0xA0, MUXP2_TEMP = 0xB0, MUXP2_AVDD = 0xC0, MUXP2_DVDD = 0xD0, MUXP2_TDACP = 0xE0, MUXP2_OPEN = 0xF0, MUXN2_AIN0 = 0x00, MUXN2_AIN1 = 0x01, MUXN2_AIN2 = 0x02, MUXN2_AIN3 = 0x03, MUXN2_AIN4 = 0x04, MUXN2_AIN5 = 0x05, MUXN2_AIN6 = 0x06, MUXN2_AIN7 = 0x07, MUXN2_AIN8 = 0x08, MUXN2_AIN9 = 0x09, MUXN2_AINCOM = 0x0A, MUXN2_TEMP = 0x0B, MUXN2_AVDD = 0x0C, MUXN2_DVDD = 0x0D, MUXN2_TDACN = 0x0E, MUXN2_OPEN = 0x0F}
}

local M = {}

function M:new(spi, cs, reset) return setmetatable({spi = spi, REG = REG, MASK = MASK, CMD = CMD, cs_mask = cs or 0x08, reset_mask = reset or 0x10}, {__index = M}) end

function M:init()
end

function M:cs(act)
  if act then self.spi:io_clr(self.cs_mask) else self.spi:io_set(self.cs_mask) end
end

function M:reset()
  self:reset_pin(true, 5) -- 5 * 200ns = 1us low pulse, > 4 clk of ~8MHz
  self:reset_pin(false)
end

function M:reset_pin(act, num)
  if self.reset_mask then for i = 1, (num or 1) do if act then self.spi:io_clr(self.reset_mask) else self.spi:io_set(self.reset_mask) end end end
end

function M:write(addr, data, len)
  len = len or 1
  local str = string.char(CMD.WREG + (addr & 0x1F), (len-1) & 0x1F)
  for i = 1, len do str = str .. string.char(data & 0xFF); data = data >> 8; end
  self:cs(true)
  self.spi:write(str)
  self:cs(false)
  self:flush()
end

function M:read(addr, len)
  len = len or 1
  local str = string.char(CMD.RREG + (addr & 0x1F), (len-1) & 0x1F) .. string.char(0):rep(len)
  self:cs(true)
  self.spi:transfer(str)
  self:cs(false)
  self:flush()
  str = self.spi:read(#str)
  local ret = 0
  for i = 3, len + 2 do ret = ret << 8; ret = ret | (str:byte(i) or 0) end
  return ret
end

function M:pulse()   self:write(REG.MODE0, self:read(REG.MODE0)  |  MASK.MODE0.RUNMODE) end
function M:cont()    self:write(REG.MODE0, self:read(REG.MODE0)  & ~MASK.MODE0.RUNMODE) end
function M:filter(x) self:write(REG.MODE1, (self:read(REG.MODE1) & ~MASK.MODE1.FILTER) | ((x & 7) << 5)) end
function M:gain(x)   self:write(REG.MODE2, (self:read(REG.MODE2) & ~MASK.MODE2.GAIN) | ((x & 7) << 4)) end
function M:rate(x)   self:write(REG.MODE2, (self:read(REG.MODE2) & ~MASK.MODE2.DR) | (x & 0x0F)) end
function M:mux(x)    self:write(REG.INPMUX, x) end
function M:start()   self:cs(true)  self.spi:write(string.char(CMD.START1))  self:cs(false) end
function M:stop()    self:cs(true)  self.spi:write(string.char(CMD.STOP1))  self:cs(false) end

function M:data_wait()    self.spi:wait_high() end

function M:flush() self.spi:flush() end

function M:data_read()
  self:cs(true)  
  self.spi:transfer(string.char(CMD.RDATA1,0,0,0,0,0,0))  
  self:cs(false)
end

function M:data()   
  str = self.spi:read(7)
  local ret = 0
  for i = 3, 6 do ret = ret << 8; ret = ret | (str:byte(i) or 0) end
  if ret >= 2^31 then ret = ret - 2^32 end
  return ret
end

return setmetatable(M, {__call = M.new})