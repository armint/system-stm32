#pragma once
#include <cmsis_device.h>

namespace Dma {

enum class Direction { PeripheralToMemory = 0, MemoryToPeripheral = DMA_CCR_DIR };

enum class DataSize { Size8Bits = 0, Size16Bits = 1, Size32Bits = 2 };

enum class ChannelPriority {
   low = 0,
   medium = DMA_CCR_PL_0,
   high = DMA_CCR_PL_1,
   very_high = (DMA_CCR_PL_0 | DMA_CCR_PL_1)
};

#if defined(DMAMUX1_Channel0_BASE)
enum class MuxInput {
   dmamux1_req_gen0 = 1,
   dmamux1_req_gen1 = 2,
   dmamux1_req_gen2 = 3,
   dmamux1_req_gen3 = 4,
   adc_dma = 5,
   i2c1_rx_dma = 10,
   i2c1_tx_dma = 11,
   spi1_rx_dma = 16,
   spi1_tx_dma = 17,
   TIM1_CH1 = 20,
   TIM1_CH2 = 21,
   TIM1_CH3 = 22,
   TIM1_CH4 = 23,
   TIM1_TRIG_COM = 24,
   TIM1_UP = 25,
   TIM3_CH1 = 32,
   TIM3_CH2 = 33,
   TIM3_CH3 = 34,
   TIM3_CH4 = 35,
   TIM3_TRIG = 36,
   TIM3_UP = 37,
   TIM16_CH1 = 44,
   TIM16_TRIG_COM = 45,
   TIM16_UP = 46,
   TIM17_CH1 = 47,
   TIM17_TRIG_COM = 48,
   TIM17_UP = 49,
   usart1_rx_dma = 50,
   usart1_tx_dma = 51,
   usart2_rx_dma = 52,
   usart2_tx_dma = 53,
};

void setMuxChannelInput(uint32_t muxChannel, MuxInput muxInput);
#endif

struct Channel {
   const uint8_t _channelPosition;
   DMA_TypeDef &_dma;
   DMA_Channel_TypeDef &_channel;
   DMA_Channel_TypeDef &getChannelTypedef() const;

   Channel(DMA_TypeDef *dma, uint8_t channelNumber);

   Channel(const Channel &) = default;
   Channel(Channel &&) = default;
   Channel &operator=(const Channel &) = delete;
   Channel &operator=(Channel &&) = delete;

   void configure(void *peripheralAddress,
                  void *memoryAddress,
                  DataSize peripheralSize,
                  DataSize memorySize,
                  Direction direction,
                  bool increaseMemAddress,
                  bool increasePeriphAddress) const;

   inline void
   configure(void *peripheralAddress, void *memoryAddress, DataSize size, Direction direction, bool increaseMemAddress, bool increasePeriphAddress) {
      configure(peripheralAddress, memoryAddress, size, size, direction, increaseMemAddress, increasePeriphAddress);
   }

   inline void enable() const {
      _channel.CCR |= DMA_CCR_EN;
   }

   inline void disable() const {
      _channel.CCR &= ~DMA_CCR_EN;
   }

   inline void enableIncreaseMemoryAddress() const {
      _channel.CCR |= DMA_CCR_MINC;
   }

   inline void disableIncreaseMemoryAddress() const {
      _channel.CCR &= ~DMA_CCR_MINC;
   }

   inline void enableIncreasePeripheralAddress() const {
      _channel.CCR |= DMA_CCR_PINC;
   }

   inline void disableIncreasePeripheralAddress() const {
      _channel.CCR &= ~DMA_CCR_PINC;
   }

   inline void setPeripheralAddress(uint32_t address) const {
      _channel.CPAR = address;
   }

   inline void setMemoryAddress(uint32_t address) const {
      _channel.CMAR = address;
   }

   inline void setDirection(Direction dir) const {
      _channel.CCR = (_channel.CCR & ~DMA_CCR_DIR) | static_cast<uint32_t>(dir);
   }

   inline void setDataSize(DataSize size) const {
      setMemoryDataSize(size);
      setPeripheralDataSize(size);
   }

   inline void setTransferCount(uint16_t count) const {
      _channel.CNDTR = count;
   }

   inline uint16_t getDataCount() const {
      return _channel.CNDTR;
   }

   inline void setMemoryDataSize(DataSize size) const {
      _channel.CCR = (_channel.CCR & ~DMA_CCR_MSIZE) | static_cast<uint32_t>(size) << DMA_CCR_MSIZE_Pos;
   }

   inline void setPeripheralDataSize(DataSize size) const {
      _channel.CCR = (_channel.CCR & ~DMA_CCR_PSIZE) | static_cast<uint32_t>(size) << DMA_CCR_PSIZE_Pos;
   }

   inline void setTransferDirection(Direction direction) const {
      _channel.CCR = (_channel.CCR & ~DMA_CCR_DIR) | static_cast<uint32_t>(direction);
   }

   inline void enableMemoryToMemory() const {
      _channel.CCR |= DMA_CCR_MEM2MEM;
   }

   inline void disableMemoryToMemory() const {
      _channel.CCR &= ~DMA_CCR_MEM2MEM;
   }

   inline void setPriority(ChannelPriority prio) const {
      _channel.CCR = (_channel.CCR & ~DMA_CCR_PL) | static_cast<uint32_t>(prio);
   }

   inline void setCircular(bool circular) const {
      if(circular) {
         _channel.CCR |= DMA_CCR_CIRC;

      } else {
         _channel.CCR &= ~DMA_CCR_CIRC;
      }
   }

   inline void enableTransferCompleteInterrupt() const {
      _channel.CCR |= DMA_CCR_TCIE;
   }

   inline void disableAllInterrupts() const {
      _channel.CCR &= ~(DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);
   }

   inline void disableTransferCompleteInterrupt() const {
      _channel.CCR &= ~DMA_CCR_TCIE;
   }

   inline void enableHalfTransferInterrupt() const {
      _channel.CCR |= DMA_CCR_HTIE;
   }

   inline void enableTransferErrorInterrupt() const {
      _channel.CCR |= DMA_CCR_TEIE;
   }

   inline void clearGlobalInterruptFlag() const {
      _dma.IFCR = DMA_IFCR_CGIF1 << (_channelPosition * 4);
   }

   inline void clearTransferCompleteInterruptFlag() const {
      _dma.IFCR = DMA_IFCR_CTCIF1 << (_channelPosition * 4);
   }

   inline void clearHalfTransferInterruptFlag() const {
      _dma.IFCR = DMA_IFCR_CHTIF1 << (_channelPosition * 4);
   }

   inline void clearTransferErrorInterruptFlag() const {
      _dma.IFCR = DMA_IFCR_CTEIF1 << (_channelPosition * 4);
   }

   inline bool isGlobalInterruptFlagSet() const {
      return _dma.ISR & (DMA_ISR_GIF1 << (_channelPosition * 4));
   }

   inline bool isTransferCompleteInterruptFlagSet() const {
      return _dma.ISR & (DMA_ISR_TCIF1 << (_channelPosition * 4));
   }

   inline bool isHalfTransferInterruptFlagSet() const {
      return _dma.ISR & (DMA_ISR_HTIF1 << (_channelPosition * 4));
   }

   inline bool isTransferErrorInterruptFlagSet() const {
      return _dma.ISR & (DMA_ISR_TEIF1 << (_channelPosition * 4));
   }
};
} // namespace Dma
