#ifdef _ENABLE_CIC
#pragma GCC push_options
#pragma GCC optimize("Os")

#include "Wire.h"
#include <avr/io.h>
extern "C" {
#include <utility/twi.h>
}

#define CLK_CH_PDN 0x80
#define CLK_CH_INT 0x40
#define CLK_CH_MS_SRC_PLL_A 0x00
#define CLK_CH_MS_SRC_PLL_B 0x20
#define CLK_CH_INV 0x10
#define CLK_CH_SRC_XTAL (0 << 2)
#define CLK_CH_SRC_CLKIN (1 << 2)
#define CLK_CH_SRC_MS (3 << 2)
#define CLK_CH_DRV_8mA 3
#define CLK_CH_DRV_2mA 0

uint8_t s_have_clock_module = 0;


//I2C通信状態を解除してA4,A5ピンを使用可能に
void DISABLE_I2C() {
  // twi_disable(); // 余計なGPIO制御があるため
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));
}

void ENABLE_I2C() {
  twi_init();
}


bool cgWriteRegister(uint8_t adr, uint8_t data) {
  Wire.beginTransmission(0x60);
  Wire.write(adr);
  Wire.write(data);
  return (Wire.endTransmission() == 0);
}
void cgReadRegisterRequest(uint8_t adr, uint8_t size) {
  Wire.beginTransmission(0x60);
  Wire.write(adr);
  Wire.endTransmission();
  Wire.requestFrom(0x60, size, true);
}

bool cgDeviceAvailable(void) {
  Wire.beginTransmission(0x60);
  return (Wire.endTransmission() == 0);
}

void cgPllReset() {
  cgReadRegisterRequest(177, 2);
  cgWriteRegister(177, 0xAC);
  cgWriteRegister(177, 0x0C);
}

void cgInit(void) {
  cgReadRegisterRequest(0, 1);

  // disable all outputs
  cgChannelDisable(0xFF);

  // power down each ch
  cgSetChannelConfig(0, CLK_CH_PDN);
  cgSetChannelConfig(1, CLK_CH_PDN);
  cgSetChannelConfig(2, CLK_CH_PDN);

  const uint8_t crystal_load = 0b10; /* 0=res / 1=6pF / 2=8pF / 3=10pF */
  cgWriteRegister(183, (crystal_load << 6) | 0b010010);

  // CLKIN DIV
  cgWriteRegister(15, (0b00 << 6) | 0b0000);

  // Spectrum Spread Disable
  cgWriteRegister(149, 0x00);

  cgPllReset();
}


bool cgSetChannelConfig(uint8_t ch, uint8_t config) {
  assert(ch < 3);
  return cgWriteRegister(16 + ch, config);
}
bool cgChannelDisable(uint8_t bitmask) {
  return cgWriteRegister(3, bitmask);
}
#define convP1(m, n, d) (128 * m + ((128 * n) / d) - 512)
#define convP2(m, n, d) (128 * n - d * ((128 * n) / d))
#define convP3(m, n, d) (d)
void clkSetVco(uint8_t ch, word m, uint32_t n, uint32_t d) {
  uint32_t P1 = convP1((uint32_t)m, n, d);
  uint32_t P2 = convP2((uint32_t)m, n, d);
  uint32_t P3 = convP3((uint32_t)m, n, d);
  uint8_t adr = ch ? 34 : 26;
  cgWriteRegister(adr + 0, (P3 >> 8) & 0xFF);
  cgWriteRegister(adr + 1, (P3)&0xFF);
  cgWriteRegister(adr + 2, (P1 >> 16) & 0xFF);
  cgWriteRegister(adr + 3, (P1 >> 8) & 0xFF);
  cgWriteRegister(adr + 4, P1);
  cgWriteRegister(adr + 5, ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0x0F));
  cgWriteRegister(adr + 6, (P2 >> 8) & 0xFF);
  cgWriteRegister(adr + 7, (P2)&0xFF);
}
void clkSetMSn(uint8_t ch, word a, uint32_t b, uint32_t c, bool div4, uint8_t r_div) {
  assert(ch < 3);
  uint8_t adr = 42 + ch * 8;
  uint32_t P1 = convP1((uint32_t)a, b, c);
  uint32_t P2 = convP2((uint32_t)a, b, c);
  uint32_t P3 = convP3((uint32_t)a, b, c);
  cgWriteRegister(adr + 0, P3 >> 8);
  cgWriteRegister(adr + 1, P3);
  cgWriteRegister(adr + 2, ((P1 >> 16) & 0x03) | (div4 ? 0x0C : 0x00) | ((r_div << 4) & 0xF0));
  cgWriteRegister(adr + 3, P1 >> 8);
  cgWriteRegister(adr + 4, P1);
  cgWriteRegister(adr + 5, ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0x0F));
  cgWriteRegister(adr + 6, P2 >> 8);
  cgWriteRegister(adr + 7, P2);
}

#define SNES_MASTER_CLK_VCO_DIV 33
//[Nintendo Cart Reader]より
void setupCloclGen(bool clk1_en, bool clk2_en, bool clk3_en, bool clk2_oc) {
  // I2C prepare
  digitalWrite(CS, HIGH);   // disable snes components
  digitalWrite(WE, HIGH);   // 18, SDA
  digitalWrite(RST, HIGH);  // 19, SCL

  ENABLE_I2C();

  lazy_wait_us(3000);

  Wire.begin();
  cgDeviceAvailable();
  s_have_clock_module = cgDeviceAvailable();

  if (s_have_clock_module) {
    cgInit();

    // https://snes.nesdev.org/wiki/Timing
    // https://fabiensanglard.net/snes_hearts/
    // SNES Master Clock: 945/44 MHz = 21.4773... MHz
    // VCO A: (SNES Master/4)*3 = 708.75 MHz (limit: 600...900)
    // 708.75 MHz = xtal 25 MHz * 28.3+
    // clkSetVco(0, 28, 35, 100);
    clkSetVco(0, 28, 7, 20);

    // SNES APU Clock: 24.576MHz
    // VCO B: 819.2 MHz = (24.576MHz / 24) * 800
    // 819.2 MHz = xtal 25 MHz * 32.768
    clkSetVco(1, 32, 768, 1000);

    // CLK0: SNES Master: VCO A / 33 = 21.4773... MHz
    clkSetMSn(0, SNES_MASTER_CLK_VCO_DIV, 0, 1, false, 0);
    if (clk2_oc) {
      // CLK1: over clock for SA-1: 6.59 MHz
      // clockgen.set_freq(659000000ULL, SI5351_CLK1);
      clkSetMSn(1, 107, 11, 20, false, 0);
    } else {
      // CLK1: 3.58 MHz fast-ROM (6-clocks per cycle)
      clkSetMSn(1, SNES_MASTER_CLK_VCO_DIV * 6, 0, 1, false, 0);
    }
    // CLK2: 3.072 MHz
    // VCO B: 819.2 MHz / (800 / 3)
    clkSetMSn(2, 266, 2, 3, false, 0);
    cgSetChannelConfig(0, CLK_CH_MS_SRC_PLL_A | CLK_CH_SRC_MS | CLK_CH_DRV_8mA | (clk1_en ? 0 : CLK_CH_PDN));
    cgSetChannelConfig(1, CLK_CH_MS_SRC_PLL_A | CLK_CH_SRC_MS | CLK_CH_DRV_8mA | (clk2_en ? 0 : CLK_CH_PDN));
    cgSetChannelConfig(2, CLK_CH_MS_SRC_PLL_B | CLK_CH_SRC_MS | CLK_CH_DRV_8mA | (clk3_en ? 0 : CLK_CH_PDN));
    cgPllReset();
    cgChannelDisable(0x0);
  }
  Wire.end();
  DISABLE_I2C();
}

bool haveClockModule(void){
  return s_have_clock_module;
}

#pragma GCC pop_options
#else
bool haveClockModule(void){
  return false;
}
#endif  /* _ENABLE_CIC */
