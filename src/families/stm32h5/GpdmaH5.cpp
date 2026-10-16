#include "GpdmaH5.h"

namespace Dma {

namespace {
// Channel register block: 0x50 behind the controller base, 0x80 per channel.
constexpr uintptr_t channelOffset = 0x50;
constexpr uintptr_t channelStride = 0x80;
constexpr uint8_t channelCount = 8;
// Upper 16 address bits go to CLBAR, bits 15:2 to CLLR.LA.
constexpr uint32_t linkBaseMask = 0xFFFF0000UL;
constexpr uint32_t linkOffsetMask = DMA_CLLR_LA_Msk;
} // namespace

LinkedListItem
LinkedListItem::make(const Config &config, const void *source, void *destination, uint16_t bytes, const LinkedListItem *next) {
   return {config.ctr1(),
           config.ctr2(),
           bytes,
           static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source)),
           static_cast<uint32_t>(reinterpret_cast<uintptr_t>(destination)),
           next == nullptr ? 0U : ((static_cast<uint32_t>(reinterpret_cast<uintptr_t>(next)) & linkOffsetMask) | updateAll)};
}

Channel::Channel(DMA_TypeDef *dma, uint8_t channelNumber)
   : _dma(*dma)
   , _channel(*reinterpret_cast<DMA_Channel_TypeDef *>(reinterpret_cast<uintptr_t>(dma) + channelOffset +
                                                        (channelNumber % channelCount) * channelStride))
   , _number(channelNumber % channelCount) {
}

bool Channel::start(const Config &config, const void *source, void *destination, uint16_t bytes, Priority priority) const {
   if(bytes == 0U || !isIdle()) {
      return false;
   }
   clearAllFlags();
   _channel.CTR1 = config.ctr1();
   _channel.CTR2 = config.ctr2();
   _channel.CBR1 = bytes;
   _channel.CSAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source));
   _channel.CDAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(destination));
   _channel.CLLR = 0; // no list: the channel completes after this block
   _channel.CCR = (_channel.CCR & ~DMA_CCR_PRIO_Msk) | (static_cast<uint32_t>(priority) << DMA_CCR_PRIO_Pos);
   _channel.CCR |= DMA_CCR_EN;
   return true;
}

bool Channel::startList(const LinkedListItem &first, Priority priority) const {
   const auto address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&first));
   if((address & 3U) != 0U || !isIdle()) {
      return false;
   }
   clearAllFlags();
   // LLI0 is an empty transfer (BNDT = 0, no flow control): UB1 loads the
   // first real item (RM0481 section 16.4.6).
   _channel.CTR2 = 0;
   _channel.CBR1 = 0;
   _channel.CLBAR = address & linkBaseMask;
   _channel.CLLR = (address & linkOffsetMask) | LinkedListItem::updateAll;
   _channel.CCR = (_channel.CCR & ~DMA_CCR_PRIO_Msk) | (static_cast<uint32_t>(priority) << DMA_CCR_PRIO_Pos);
   _channel.CCR |= DMA_CCR_EN;
   return true;
}

void Channel::abort() const {
   _channel.CCR |= DMA_CCR_RESET;
   while((_channel.CCR & DMA_CCR_RESET) != 0U) {
   }
   clearAllFlags();
}

} // namespace Dma
