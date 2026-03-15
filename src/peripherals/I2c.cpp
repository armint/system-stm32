#include "I2c.h"
#include "Rcc.h"

namespace hw {

// Fallback definitions for I2Cv2 bits if missing in CMSIS headers
#ifndef I2C_OAR1_OA1MODE
#define I2C_OAR1_OA1MODE (1UL << 10)
#endif
#ifndef I2C_OAR1_OA1EN
#define I2C_OAR1_OA1EN (1UL << 15)
#endif
#ifndef I2C_CR2_NBYTES_Pos
#define I2C_CR2_NBYTES_Pos 16
#endif
#ifndef I2C_CR2_SADD_Pos
#define I2C_CR2_SADD_Pos 0
#endif
#ifndef I2C_CR2_NBYTES
#define I2C_CR2_NBYTES (0xFFUL << 16)
#endif
#ifndef I2C_CR2_SADD
#define I2C_CR2_SADD (0x3FFUL << 0)
#endif

void I2c::configureMaster(uint32_t clockFrequency,
                           Mode mode,
                           bool addressMode,
                           uint16_t ownAddress) const {
   bool wasEnabled = isEnabled();
   if(wasEnabled)
      disable();

   // Configure clock timing based on clock frequency
   uint32_t clockFreq = getClockFrequency();

   if(mode == Mode::Standard) {
      // Standard mode: calculate timing for desired clock frequency
      // Simple approximation - adjust based on actual clock
      if(clockFreq >= 40000000) {
         i2c_.TIMINGR = 0x107076B7;  // 100kHz @ 48MHz
      } else if(clockFreq >= 25000000) {
         i2c_.TIMINGR = 0x10504494;  // 100kHz @ 25MHz
      } else if (clockFreq >= 16000000) {
          i2c_.TIMINGR = 0x00303D5B; // 100kHz @ 16MHz
      } else if (clockFreq >= 8000000) {
          i2c_.TIMINGR = 0x00101D2D; // 100kHz @ 8MHz
      } else {
         i2c_.TIMINGR = 0x00000000;  // Fallback
      }
   } else {
      // Fast mode
      if(clockFreq >= 40000000) {
         i2c_.TIMINGR = 0x0060362A;  // 400kHz @ 48MHz
      } else if(clockFreq >= 25000000) {
         i2c_.TIMINGR = 0x0050281D;  // 400kHz @ 25MHz
      } else {
         i2c_.TIMINGR = 0x00000000;  // Fallback
      }
   }

   // Configure Own Address 1 if provided
   if(ownAddress != 0) {
       uint32_t oar1 = 0;
       if(addressMode) {
           // 10-bit addressing
           oar1 |= I2C_OAR1_OA1MODE;
           oar1 |= (ownAddress & 0x3FF);
       } else {
           // 7-bit addressing
           // OA1[7:1] contains the 7-bit address
           oar1 |= (ownAddress << 1) & 0xFE;
       }
       oar1 |= I2C_OAR1_OA1EN;
       i2c_.OAR1 = oar1;
   } else {
       i2c_.OAR1 = 0;
   }

   // Disable Own Address 2
   i2c_.OAR2 = 0;

   // Ensure 7-bit addressing mode for master target addressing by default
   i2c_.CR2 &= ~I2C_CR2_ADD10;

   if(wasEnabled)
      enable();
}

void I2c::configureSlave(bool addressMode, uint16_t ownAddress) const {
   bool wasEnabled = isEnabled();
   if(wasEnabled)
      disable();

   // Configure Own Address 1
   uint32_t oar1 = 0;
   if(addressMode) {
      // 10-bit addressing
      oar1 |= I2C_OAR1_OA1MODE;
      oar1 |= (ownAddress & 0x3FF);
   } else {
      // 7-bit addressing
      oar1 |= (ownAddress << 1) & 0xFE;
   }
   oar1 |= I2C_OAR1_OA1EN;
   i2c_.OAR1 = oar1;

   // Disable Own Address 2
   i2c_.OAR2 = 0;

   if(wasEnabled)
      enable();
}

void I2c::setTransferConfig(uint8_t address,
                             uint8_t numberOfBytes,
                             TransferDirection direction,
                             bool autoEnd) const {
   // Clear NBYTES, SADD, RD_WRN, START, STOP, AUTOEND, RELOAD
   uint32_t cr2 = i2c_.CR2 & ~(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_AUTOEND | I2C_CR2_RELOAD);

   // SADD[7:1] is address for 7-bit mode
   cr2 |= (static_cast<uint32_t>(address) << 1) & I2C_CR2_SADD;

   cr2 |= (static_cast<uint32_t>(numberOfBytes) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES;
   cr2 |= static_cast<uint32_t>(direction);
   if(autoEnd) {
      cr2 |= I2C_CR2_AUTOEND;
   }

   i2c_.CR2 = cr2;
}

void I2c::transmit(uint8_t slaveAddress,
                   const uint8_t *data,
                   uint8_t length,
                   bool stopCondition) const {
   // Wait for bus to be idle
   while(isBusBusy()) {}

   // Configure transfer
   setTransferConfig(slaveAddress,
                     length,
                     TransferDirection::Tx,
                     stopCondition);

   // Generate start condition
   generateStart();

   // Transmit data
   for(uint8_t i = 0; i < length; ++i) {
      while(!isTxEmpty()) {
         // Check for NACK or errors
         if(isNackReceived()) {
             clearNackFlag();
             return;
         }
      }
      write(data[i]);
   }

   // Wait for transfer to complete
   while(!isTransferComplete()) {
      if(isNackReceived()) {
          clearNackFlag();
          return;
      }
   }

   // Clear flags if autoEnd is disabled
   if(!stopCondition) {
      // For manual stop, usually handled by caller or next transfer
   }
}

void I2c::receive(uint8_t slaveAddress,
                  uint8_t *data,
                  uint8_t length,
                  bool stopCondition) const {
   // Wait for bus to be idle
   while(isBusBusy()) {}

   // Configure transfer
   setTransferConfig(slaveAddress,
                     length,
                     TransferDirection::Rx,
                     stopCondition);

   // Generate start condition
   generateStart();

   // Receive data
   for(uint8_t i = 0; i < length; ++i) {
      while(!isRxNotEmpty()) {
         if(isNackReceived()) {
             clearNackFlag();
             return;
         }
      }
      data[i] = read();
   }

   // Wait for transfer to complete
   while(!isTransferComplete()) {
       if(isNackReceived()) {
             clearNackFlag();
             return;
         }
   }
}

uint32_t I2c::getClockFrequency() const {
#if defined(RCC_CFGR_PPRE2)
   if(reinterpret_cast<uintptr_t>(&i2c_) == I2C1_BASE) {
      return Rcc::getAPBClock();
   }
#endif
#if defined(RCC_CFGR_PPRE1)
   if(reinterpret_cast<uintptr_t>(&i2c_) == I2C2_BASE) {
      return Rcc::getAPBClock();
   }
#endif
   return Rcc::getAPBClock();
}

} // namespace hw
