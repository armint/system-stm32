#pragma once

#include <cmsis_device.h>
#include <cstdint>

/**
 * @brief I2C Peripheral access class.
 *
 * This class provides a thin wrapper around the STM32 I2C peripheral registers.
 * It follows the pattern used for other peripheral classes in this project.
 *
 * Supports STM32L0, STM32G0, and other series with the I2Cv2 peripheral.
 */
namespace hw {

struct I2c {
   enum class Mode {
      Standard,
      Fast
   };

   enum class StartCondition : uint32_t {
      None = 0,
      Generate = I2C_CR2_START,
      GenerateStopAfter = I2C_CR2_START | I2C_CR2_AUTOEND
   };

   enum class TransferDirection : uint32_t {
      Tx = 0,
      Rx = I2C_CR2_RD_WRN
   };

   I2C_TypeDef &i2c_;

   explicit I2c(I2C_TypeDef *i2cBase)
      : i2c_(*i2cBase) {}

   I2c(const I2c &) = default;
   I2c(I2c &&) = default;
   I2c &operator=(const I2c &) = delete;
   I2c &operator=(I2c &&) = delete;

   /**
    * @brief Configures the I2C peripheral in master mode.
    * @param clockFrequency Desired SCL clock frequency in Hz.
    * @param mode Mode (Standard or Fast).
    * @param addressMode Address mode for slave addressing.
    * @param ownAddress Own 7-bit or 10-bit address.
    */
   void configureMaster(uint32_t clockFrequency = 100000,
                        Mode mode = Mode::Standard,
                        bool addressMode = false,
                        uint16_t ownAddress = 0) const;

   /**
    * @brief Configures the I2C peripheral in slave mode.
    * @param addressMode Address mode (7-bit or 10-bit).
    * @param ownAddress Own 7-bit or 10-bit address.
    */
   void configureSlave(bool addressMode = false, uint16_t ownAddress = 0) const;

   /**
    * @brief Enables the I2C peripheral.
    */
   inline void enable() const {
      i2c_.CR1 |= I2C_CR1_PE;
   }

   /**
    * @brief Disables the I2C peripheral.
    */
   inline void disable() const {
      i2c_.CR1 &= ~I2C_CR1_PE;
   }

   /**
    * @brief Checks if the I2C peripheral is enabled.
    * @return true if enabled, false otherwise.
    */
   inline bool isEnabled() const {
      return (i2c_.CR1 & I2C_CR1_PE) != 0;
   }

   /**
    * @brief Generates a START condition.
    */
   inline void generateStart() const {
      i2c_.CR2 |= I2C_CR2_START;
   }

   /**
    * @brief Generates a STOP condition.
    */
   inline void generateStop() const {
      i2c_.CR2 |= I2C_CR2_STOP;
   }

   /**
    * @brief Transmits a single data byte.
    * @param data Data to transmit.
    */
   inline void write(uint8_t data) const {
      i2c_.TXDR = data;
   }

   /**
    * @brief Receives a single data byte.
    * @return Received data.
    */
   inline uint8_t read() const {
      return static_cast<uint8_t>(i2c_.RXDR);
   }

   /**
    * @brief Checks if the transmit data register is empty (TXE).
    * Note: In I2Cv2 (L0/G0), TXE is set when TXDR is empty.
    * For transmitting data, one should typically wait for TXIS (Transmit Interrupt Status)
    * which indicates the TXDR register is empty and ready for the next byte.
    * @return true if TXE is set, false otherwise.
    */
   inline bool isTxEmpty() const {
      // For I2Cv2, TXIS (Transmit Interrupt Status) is generally the flag to check before writing to TXDR
      // However, TXE is also available. Let's use TXIS as it is more specific for "Ready to write next byte"
#if defined(I2C_ISR_TXIS)
      return (i2c_.ISR & I2C_ISR_TXIS) != 0;
#else
      return (i2c_.ISR & I2C_ISR_TXE) != 0;
#endif
   }

   /**
    * @brief Checks if the receive data register is not empty.
    * @return true if RXDR has data, false otherwise.
    */
   inline bool isRxNotEmpty() const {
      return (i2c_.ISR & I2C_ISR_RXNE) != 0;
   }

   /**
    * @brief Checks if a byte transfer is complete.
    * @return true if transfer complete, false otherwise.
    */
   inline bool isTransferComplete() const {
      return (i2c_.ISR & I2C_ISR_TC) != 0;
   }

   /**
    * @brief Checks if a STOP condition has been sent.
    * @return true if STOP sent, false otherwise.
    */
   inline bool isStopSent() const {
      return (i2c_.ISR & I2C_ISR_STOPF) != 0;
   }

   /**
    * @brief Checks if a NACK has been received.
    * @return true if NACK received, false otherwise.
    */
   inline bool isNackReceived() const {
      return (i2c_.ISR & I2C_ISR_NACKF) != 0;
   }

   /**
    * @brief Checks if the bus is busy.
    * @return true if bus is busy, false otherwise.
    */
   inline bool isBusBusy() const {
      return (i2c_.ISR & I2C_ISR_BUSY) != 0;
   }

   /**
    * @brief Checks if arbitration has been lost.
    * @return true if arbitration lost, false otherwise.
    */
   inline bool isArbitrationLost() const {
      return (i2c_.ISR & I2C_ISR_ARLO) != 0;
   }

   /**
    * @brief Checks if a bus error has occurred.
    * @return true if bus error, false otherwise.
    */
   inline bool isBusError() const {
      return (i2c_.ISR & I2C_ISR_BERR) != 0;
   }

   /**
    * @brief Checks if a CRC error has occurred.
    * @return true if CRC error, false otherwise.
    */
   inline bool isCrcError() const {
#if defined(I2C_ISR_PECERR)
      return (i2c_.ISR & I2C_ISR_PECERR) != 0;
#else
      return false;
#endif
   }

   /**
    * @brief Clears the STOP condition flag.
    */
   inline void clearStopFlag() const {
      i2c_.ICR |= I2C_ICR_STOPCF;
   }

   /**
    * @brief Clears the NACK flag.
    */
   inline void clearNackFlag() const {
      i2c_.ICR |= I2C_ICR_NACKCF;
   }

   /**
    * @brief Clears the bus error flag.
    */
   inline void clearBusErrorFlag() const {
      i2c_.ICR |= I2C_ICR_BERRCF;
   }

   /**
    * @brief Clears the arbitration lost flag.
    */
   inline void clearArbitrationLostFlag() const {
      i2c_.ICR |= I2C_ICR_ARLOCF;
   }

   /**
    * @brief Clears the CRC error flag.
    */
   inline void clearCrcErrorFlag() const {
#if defined(I2C_ICR_PECERRCF)
      i2c_.ICR |= I2C_ICR_PECERRCF;
#endif
   }

   /**
    * @brief Clears all status flags.
    */
   inline void clearAllFlags() const {
      uint32_t mask = I2C_ICR_STOPCF;
#ifdef I2C_ICR_NACKCF
      mask |= I2C_ICR_NACKCF;
#endif
#ifdef I2C_ICR_BERRCF
      mask |= I2C_ICR_BERRCF;
#endif
#ifdef I2C_ICR_ARLOCF
      mask |= I2C_ICR_ARLOCF;
#endif
#ifdef I2C_ICR_PECERRCF
      mask |= I2C_ICR_PECERRCF;
#endif
      i2c_.ICR = mask;
   }

   // =============================================================
   // Interrupt Control
   // =============================================================

   /**
    * @brief Enables the transmit interrupt.
    */
   inline void enableTxInterrupt() const {
      i2c_.CR1 |= I2C_CR1_TXIE;
   }

   /**
    * @brief Disables the transmit interrupt.
    */
   inline void disableTxInterrupt() const {
      i2c_.CR1 &= ~I2C_CR1_TXIE;
   }

   /**
    * @brief Checks if the transmit interrupt is enabled.
    * @return true if enabled, false otherwise.
    */
   inline bool isTxInterruptEnabled() const {
      return (i2c_.CR1 & I2C_CR1_TXIE) != 0;
   }

   /**
    * @brief Enables the receive interrupt.
    */
   inline void enableRxInterrupt() const {
      i2c_.CR1 |= I2C_CR1_RXIE;
   }

   /**
    * @brief Disables the receive interrupt.
    */
   inline void disableRxInterrupt() const {
      i2c_.CR1 &= ~I2C_CR1_RXIE;
   }

   /**
    * @brief Checks if the receive interrupt is enabled.
    * @return true if enabled, false otherwise.
    */
   inline bool isRxInterruptEnabled() const {
      return (i2c_.CR1 & I2C_CR1_RXIE) != 0;
   }

   /**
    * @brief Enables the transfer complete interrupt.
    */
   inline void enableTransferCompleteInterrupt() const {
      i2c_.CR1 |= I2C_CR1_TCIE;
   }

   /**
    * @brief Disables the transfer complete interrupt.
    */
   inline void disableTransferCompleteInterrupt() const {
      i2c_.CR1 &= ~I2C_CR1_TCIE;
   }

   /**
    * @brief Checks if the transfer complete interrupt is enabled.
    * @return true if enabled, false otherwise.
    */
   inline bool isTransferCompleteInterruptEnabled() const {
      return (i2c_.CR1 & I2C_CR1_TCIE) != 0;
   }

   /**
    * @brief Enables the error interrupt.
    */
   inline void enableErrorInterrupt() const {
      i2c_.CR1 |= I2C_CR1_ERRIE;
   }

   /**
    * @brief Disables the error interrupt.
    */
   inline void disableErrorInterrupt() const {
      i2c_.CR1 &= ~I2C_CR1_ERRIE;
   }

   /**
    * @brief Checks if the error interrupt is enabled.
    * @return true if enabled, false otherwise.
    */
   inline bool isErrorInterruptEnabled() const {
      return (i2c_.CR1 & I2C_CR1_ERRIE) != 0;
   }

   // =============================================================
   // DMA Control
   // =============================================================

   /**
    * @brief Enables DMA transmission.
    */
   inline void enableTxDma() const {
      i2c_.CR1 |= I2C_CR1_TXDMAEN;
   }

   /**
    * @brief Disables DMA transmission.
    */
   inline void disableTxDma() const {
      i2c_.CR1 &= ~I2C_CR1_TXDMAEN;
   }

   /**
    * @brief Enables DMA reception.
    */
   inline void enableRxDma() const {
      i2c_.CR1 |= I2C_CR1_RXDMAEN;
   }

   /**
    * @brief Disables DMA reception.
    */
   inline void disableRxDma() const {
      i2c_.CR1 &= ~I2C_CR1_RXDMAEN;
   }

   // =============================================================
   // Configuration Helpers
   // =============================================================

   /**
    * @brief Sets the transfer configuration for a transaction.
    * @param address 7-bit or 10-bit slave address.
    * @param numberOfBytes Number of bytes to transfer.
    * @param direction Transfer direction (Tx or Rx).
    * @param autoEnd If true, automatically generates STOP condition.
    */
   void setTransferConfig(uint8_t address,
                          uint8_t numberOfBytes,
                          TransferDirection direction,
                          bool autoEnd = false) const;

   /**
    * @brief Starts a master transmission.
    * @param slaveAddress 7-bit slave address.
    * @param data Pointer to data buffer.
    * @param length Number of bytes to transmit.
    * @param stopCondition If true, generates STOP after transfer.
    */
   void transmit(uint8_t slaveAddress,
                 const uint8_t *data,
                 uint8_t length,
                 bool stopCondition = true) const;

   /**
    * @brief Starts a master reception.
    * @param slaveAddress 7-bit slave address.
    * @param data Pointer to data buffer.
    * @param length Number of bytes to receive.
    * @param stopCondition If true, generates STOP after transfer.
    */
   void receive(uint8_t slaveAddress,
                uint8_t *data,
                uint8_t length,
                bool stopCondition = true) const;

   /**
    * @brief Returns the clock frequency of the APB bus the I2C is connected to.
    */
   uint32_t getClockFrequency() const;
};

} // namespace hw
