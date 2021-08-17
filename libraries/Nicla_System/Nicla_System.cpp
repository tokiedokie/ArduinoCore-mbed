#include "Nicla_System.h"

#if defined __has_include
#  if __has_include (<Arduino_BHY2.h>)
#    define NO_NEED_FOR_WATCHDOG_THREAD
#  else
#    include "rtos.h"
#  endif
#endif


RGBled nicla::leds;
BQ25120A nicla::_pmic;
rtos::Mutex nicla::i2c_mutex;

void nicla::pingI2CThd() {
  while(1) {
    // already protected by a mutex on Wire operations
    readLDOreg();
    delay(10000);
  }
}

bool nicla::begin()
{
  pinMode(P0_10, OUTPUT);
  digitalWrite(P0_10, HIGH);
  Wire1.begin();
#ifndef NO_NEED_FOR_WATCHDOG_THREAD
  static rtos::Thread th(osPriorityHigh, 1024, nullptr, "ping_thread");
  th.start(&nicla::pingI2CThd);
#endif
}

void nicla::enableCD()
{
  pinMode(p25, OUTPUT);
  digitalWrite(p25, HIGH);
}

void nicla::disableCD()
{
  pinMode(p25, OUTPUT);
  digitalWrite(p25, LOW);
}

  /*
      LDO reg:
      |   B7   |  B6   |  B5   |  B4   |  B3   |  B2   | B1  | B0  |
      | EN_LDO | LDO_4 | LDO_3 | LDO_2 | LDO_1 | LDO_0 |  X  |  X  |

      Conversion function:
      LDO = 0.8V + LDO_CODE * 100mV

      - for LDO = 3.3V:
          - set LCO_CODE = 25 (0x19)
          - shift to lef by 2 positions: (0x19 << 2) = 0x64
          - set EN_LDO: 0xE4
      - for LDO = 1.8V:
          - set LCO_CODE = 10 (0x0A)
          - shift to lef by 2 positions: (0x0A << 2) = 0x28
          - set EN_LDO: 0xA8
  */

bool nicla::enable3V3LDO()
{
  enableCD();
  uint8_t ldo_reg = 0xE4;
  _pmic.writeByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL, ldo_reg);
  if (_pmic.readByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL) != ldo_reg) {
    disableCD();
    return false;
  }
  disableCD();
  return true;
}

bool nicla::enable1V8LDO()
{
  enableCD();
  uint8_t ldo_reg = 0xA8;
  _pmic.writeByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL, ldo_reg);
  if (_pmic.readByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL) != ldo_reg) {
    disableCD();
    return false;
  }
  disableCD();
  return true;
}

bool nicla::disableLDO()
{
  enableCD();
  uint8_t ldo_reg = _pmic.readByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL);
  ldo_reg &= 0x7F;
  _pmic.writeByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL, ldo_reg);
  if (_pmic.readByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL) != ldo_reg) {
    disableCD();
    return false;
  }
  disableCD();
  return true;
}

uint8_t nicla::readLDOreg()
{
  enableCD();
  uint8_t ldo_reg = _pmic.readByte(BQ25120A_ADDRESS, BQ25120A_LDO_CTRL);
  disableCD();
  return ldo_reg;
}