#pragma once

// GPDMA of the STM32H5 (RM0481 chapter 16). peripherals/Dma.h includes this
// file on the H5. The GPDMA is not the L0/G0 DMA: a channel has a source and
// a destination address, a request selector in the channel itself (no
// DMAMUX), a block size in bytes and a linked-list engine. Thus the H5 has
// its own header, in the same namespace as the shared Dma.h.
//
// Channels are numbered from 0, as in the RM and in the IRQ names
// (GPDMA1_Channel0_IRQn). Channels 0 to 3 have a 8-byte FIFO, 4 to 7 a 32-byte
// FIFO. The 2D addressing of channels 6 and 7 is not used. Both AHB ports
// stay on port 0.

#include <cmsis_device.h>
#include <cstddef>
#include <cstdint>

namespace Dma {

enum class Direction : uint8_t {
   PeripheralToMemory,
   MemoryToPeripheral,
   MemoryToMemory, // started by software, runs at full speed
};

// Binary logarithm of the width in bytes: the SDW_LOG2 and DDW_LOG2 values.
enum class DataSize : uint8_t { Size8Bits = 0, Size16Bits = 1, Size32Bits = 2 };

// 0 to 2: low priority with a growing weight, 3: high priority.
enum class Priority : uint8_t { low = 0, medium = 1, high = 2, very_high = 3 };

// When the transfer complete flag (TCF) is set. In run-to-completion mode
// the hardware always sets TCF at the end of the channel.
enum class CompleteEvent : uint8_t { Block = 0, LinkedListItem = 2, Channel = 3 };

// GPDMA_CxTR2.REQSEL, RM0481 table 142. Only the requests of the
// peripherals that system-stm32 drives on the H5.
enum class Request : uint8_t {
   adc1 = 0,
   adc2 = 1,
   dac1_ch1 = 2,
   dac1_ch2 = 3,
   tim6_upd = 4,
   tim7_upd = 5,
   spi1_rx = 6,
   spi1_tx = 7,
   spi2_rx = 8,
   spi2_tx = 9,
   spi3_rx = 10,
   spi3_tx = 11,
   i2c1_rx = 12,
   i2c1_tx = 13,
   i2c2_rx = 15,
   i2c2_tx = 16,
   usart1_rx = 21,
   usart1_tx = 22,
   usart2_rx = 23,
   usart2_tx = 24,
   usart3_rx = 25,
   usart3_tx = 26,
   uart4_rx = 27,
   uart4_tx = 28,
   uart5_rx = 29,
   uart5_tx = 30,
   lpuart1_rx = 45,
   lpuart1_tx = 46,
   spi4_rx = 47,
   spi4_tx = 48,
   tim1_cc1 = 58,
   tim1_cc2 = 59,
   tim1_cc3 = 60,
   tim1_cc4 = 61,
   tim1_upd = 62,
   tim1_trg = 63,
   tim1_com = 64,
   tim2_cc1 = 72,
   tim2_cc2 = 73,
   tim2_cc3 = 74,
   tim2_cc4 = 75,
   tim2_upd = 76,
   tim3_cc1 = 77,
   tim3_cc2 = 78,
   tim3_cc3 = 79,
   tim3_cc4 = 80,
   tim3_upd = 81,
   tim3_trg = 82,
   tim15_cc1 = 94,
   tim15_upd = 95,
};

/**
 * @brief Settings of one transfer: the values of CTR1 and CTR2.
 * For PeripheralToMemory the source is the peripheral, for MemoryToPeripheral
 * the destination. @c request is ignored for MemoryToMemory.
 */
struct Config {
   Direction direction = Direction::MemoryToMemory;
   Request request = Request::adc1;
   DataSize sourceSize = DataSize::Size8Bits;
   DataSize destinationSize = DataSize::Size8Bits;
   bool sourceIncrement = false;
   bool destinationIncrement = false;
   CompleteEvent completeEvent = CompleteEvent::Block;

   /// CTR1 value. Different widths pack or unpack the data stream (PAM = 2),
   /// so the destination gets the same bytes as the source read.
   constexpr uint32_t ctr1() const {
      const uint32_t pam = sourceSize != destinationSize ? (2UL << DMA_CTR1_PAM_Pos) : 0U;
      return pam | (static_cast<uint32_t>(sourceSize) << DMA_CTR1_SDW_LOG2_Pos) | (sourceIncrement ? DMA_CTR1_SINC : 0U) |
             (static_cast<uint32_t>(destinationSize) << DMA_CTR1_DDW_LOG2_Pos) | (destinationIncrement ? DMA_CTR1_DINC : 0U);
   }

   /// CTR2 value. DREQ = 1 makes the destination the request source.
   constexpr uint32_t ctr2() const {
      const uint32_t tcem = static_cast<uint32_t>(completeEvent) << DMA_CTR2_TCEM_Pos;
      if(direction == Direction::MemoryToMemory) {
         return DMA_CTR2_SWREQ | tcem;
      }
      return (static_cast<uint32_t>(request) << DMA_CTR2_REQSEL_Pos) |
             (direction == Direction::MemoryToPeripheral ? DMA_CTR2_DREQ : 0U) | tcem;
   }
};

/**
 * @brief Linked-list item (LLI) of a channel without 2D addressing
 * (channels 0 to 5). The hardware reads the words in this order. An item sits
 * in SRAM, 4-byte aligned, in the same 64 KiB region as every other item of
 * the list and as the first item (CLBAR holds the upper 16 address bits). The
 * driver cannot check this: align the item array to a power of 2 that is at
 * least its size.
 */
struct alignas(4) LinkedListItem {
   uint32_t ctr1;
   uint32_t ctr2;
   uint32_t cbr1;
   uint32_t csar;
   uint32_t cdar;
   uint32_t cllr;

   /// All fields of the next item replace the channel registers.
   static constexpr uint32_t updateAll = DMA_CLLR_UT1 | DMA_CLLR_UT2 | DMA_CLLR_UB1 | DMA_CLLR_USA | DMA_CLLR_UDA | DMA_CLLR_ULL;

   /**
    * @brief Builds an item that transfers @p bytes and then runs @p next.
    * @param next Following item. nullptr ends the list; the item may point to
    *        itself or to an earlier item for a circular transfer.
    */
   static LinkedListItem make(const Config &config, const void *source, void *destination, uint16_t bytes, const LinkedListItem *next);
};

struct Channel {
   DMA_TypeDef &_dma;
   DMA_Channel_TypeDef &_channel;
   const uint8_t _number;

   /// @param dma GPDMA1 or GPDMA2. @param channelNumber 0 to 7, not checked.
   Channel(DMA_TypeDef *dma, uint8_t channelNumber);

   Channel(const Channel &) = default;
   Channel(Channel &&) = default;
   Channel &operator=(const Channel &) = delete;
   Channel &operator=(Channel &&) = delete;

   /**
    * @brief Single transfer: programs the channel and starts it.
    * The channel must be idle. @p bytes counts source bytes: a multiple of the
    * wider of the two data widths, 1 to 65535. The data pointers must stay valid until the
    * transfer ends.
    * @return false if the channel is not idle and disabled, or @p bytes is 0.
    */
   bool start(const Config &config, const void *source, void *destination, uint16_t bytes, Priority priority = Priority::low) const;

   /**
    * @brief Linked-list transfer: starts the list at @p first.
    * The first item runs after an empty transfer of the channel registers.
    * @p first must be 4-byte aligned. The channel must be idle.
    * @return false if the channel is not idle and disabled.
    */
   bool startList(const LinkedListItem &first, Priority priority = Priority::low) const;

   /// Suspends a running transfer, then resets the channel (FIFO and state).
   /// The address and block registers must be programmed again.
   void abort() const;

   /// Reset of a channel in steady state; used by start() and abort().
   void reset() const;

   bool isIdle() const {
      return (_channel.CSR & DMA_CSR_IDLEF) != 0U;
   }
   /// Free for a new transfer: nothing runs and EN is clear. A transfer error
   /// also leaves the channel here, with an error flag set.
   bool isFree() const {
      return isIdle() && !isEnabled();
   }
   bool isEnabled() const {
      return (_channel.CCR & DMA_CCR_EN) != 0U;
   }
   /// Source bytes of the current block that are not read yet.
   uint16_t remainingBytes() const {
      return static_cast<uint16_t>((_channel.CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos);
   }

   void enableTransferCompleteInterrupt() const {
      _channel.CCR |= DMA_CCR_TCIE;
   }
   void disableTransferCompleteInterrupt() const {
      _channel.CCR &= ~DMA_CCR_TCIE;
   }
   void enableHalfTransferInterrupt() const {
      _channel.CCR |= DMA_CCR_HTIE;
   }
   /// Data transfer error, user setting error and update link error.
   void enableErrorInterrupts() const {
      _channel.CCR |= DMA_CCR_DTEIE | DMA_CCR_USEIE | DMA_CCR_ULEIE;
   }
   void disableAllInterrupts() const {
      _channel.CCR &= ~(DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_DTEIE | DMA_CCR_USEIE | DMA_CCR_ULEIE);
   }

   bool isTransferCompleteFlagSet() const {
      return (_channel.CSR & DMA_CSR_TCF) != 0U;
   }
   bool isHalfTransferFlagSet() const {
      return (_channel.CSR & DMA_CSR_HTF) != 0U;
   }
   /// Data transfer error, user setting error or update link error.
   bool isErrorFlagSet() const {
      return (_channel.CSR & (DMA_CSR_DTEF | DMA_CSR_USEF | DMA_CSR_ULEF)) != 0U;
   }
   void clearTransferCompleteFlag() const {
      _channel.CFCR = DMA_CFCR_TCF;
   }
   void clearHalfTransferFlag() const {
      _channel.CFCR = DMA_CFCR_HTF;
   }
   void clearErrorFlags() const {
      _channel.CFCR = DMA_CFCR_DTEF | DMA_CFCR_USEF | DMA_CFCR_ULEF;
   }
   void clearAllFlags() const {
      _channel.CFCR = DMA_CFCR_TCF | DMA_CFCR_HTF | DMA_CFCR_DTEF | DMA_CFCR_ULEF | DMA_CFCR_USEF | DMA_CFCR_SUSPF | DMA_CFCR_TOF;
   }
};

} // namespace Dma
