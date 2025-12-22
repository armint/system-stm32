#include "Dma.h"
// #include <cassert>

namespace Dma {
Channel::Channel(DMA_TypeDef *dma, uint8_t channelNumber)
   : _channelPosition(channelNumber - 1)
   , _dma(*dma)
   , _channel(getChannelTypedef()) {
   //   assert(channelNumber > 0 && channelNumber < 8);
}

void Channel::configure(void *peripheralAddress,
                        void *memoryAddress,
                        DataSize peripheralSize,
                        DataSize memorySize,
                        Direction direction,
                        bool increaseMemAddress,
                        bool increasePeriphAddress) const {
   setPeripheralAddress((uintptr_t)peripheralAddress);
   setMemoryAddress((uintptr_t)memoryAddress);
   setMemoryDataSize(memorySize);
   setPeripheralDataSize(peripheralSize);
   _channel.CCR = (_channel.CCR & ~((uint32_t)DMA_CCR_MINC | DMA_CCR_PINC | DMA_CCR_DIR)) |
                  (increaseMemAddress ? DMA_CCR_MINC : 0) | (increasePeriphAddress ? DMA_CCR_PINC : 0) |
                  static_cast<uint32_t>(direction);
}

DMA_Channel_TypeDef &Channel::getChannelTypedef() const {
   uintptr_t chAddress = DMA1_Channel1_BASE + _channelPosition * (DMA1_Channel2_BASE - DMA1_Channel1_BASE);
   return *reinterpret_cast<DMA_Channel_TypeDef *>(chAddress);
}

#if defined(DMAMUX1_Channel0_BASE)
void setMuxChannelInput(uint32_t muxChannel, MuxInput muxInput) {
   uint32_t muxChannelBase = DMAMUX1_Channel0_BASE + 4 * muxChannel;
   auto *dmaMuxChannel = reinterpret_cast<DMAMUX_Channel_TypeDef *>(muxChannelBase);
   dmaMuxChannel->CCR =
   (dmaMuxChannel->CCR & ~(DMAMUX_CxCR_DMAREQ_ID_Msk)) | (static_cast<uint32_t>(muxInput) << DMAMUX_CxCR_DMAREQ_ID_Pos);
}
#endif
} // namespace Dma
