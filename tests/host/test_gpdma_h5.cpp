// Host tests for the GPDMA driver of the STM32H5: the CTR1/CTR2 encoding,
// the linked-list items and the register writes of Channel. The register
// image is plain memory, so the test checks what the driver writes, not what
// the hardware does with it.

#include "GpdmaH5.h"

#include <cstdio>
#include <cstring>

namespace {

int failures = 0;

#define CHECK(cond)                                                  \
   do {                                                              \
      if(!(cond)) {                                                  \
         std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
         ++failures;                                                 \
      }                                                              \
   } while(0)

// A controller with 8 channels: the channel block starts at 0x50, 0x80 apart.
struct FakeGpdma {
   alignas(8) uint8_t bytes[0x50 + 8 * 0x80] = {};
   DMA_TypeDef *dma() {
      return reinterpret_cast<DMA_TypeDef *>(bytes);
   }
   DMA_Channel_TypeDef &channel(unsigned n) {
      return *reinterpret_cast<DMA_Channel_TypeDef *>(bytes + 0x50 + n * 0x80);
   }
};

uint32_t addr(const void *p) {
   return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
}

void testConfig() {
   Dma::Config c;
   c.direction = Dma::Direction::MemoryToPeripheral;
   c.request = Dma::Request::spi1_tx;
   c.sourceSize = Dma::DataSize::Size32Bits;
   c.destinationSize = Dma::DataSize::Size8Bits;
   c.sourceIncrement = true;
   // SDW_LOG2 = 2 at bits 1:0, SINC bit 3, DDW_LOG2 at bits 17:16.
   CHECK(c.ctr1() == (2U | (1U << 3)));
   // REQSEL = 7 (spi1_tx), DREQ bit 10: the destination is the request source.
   CHECK(c.ctr2() == (7U | (1U << 10)));

   c.direction = Dma::Direction::PeripheralToMemory;
   c.request = Dma::Request::usart1_rx;
   CHECK(c.ctr2() == 21U); // DREQ = 0: the source is the request source

   c.direction = Dma::Direction::MemoryToMemory;
   c.completeEvent = Dma::CompleteEvent::Channel;
   CHECK(c.ctr2() == ((1U << 9) | (3UL << 30))); // SWREQ, TCEM = 3
}

void testSingle() {
   FakeGpdma gpdma;
   Dma::Channel ch(gpdma.dma(), 3);
   auto &regs = gpdma.channel(3);
   uint8_t src[8] = {};
   uint8_t dst[8] = {};
   Dma::Config c;
   c.sourceIncrement = true;
   c.destinationIncrement = true;

   CHECK(!ch.start(c, src, dst, 8)); // CSR.IDLEF is 0: busy
   regs.CSR = DMA_CSR_IDLEF;
   CHECK(!ch.start(c, src, dst, 0)); // empty block
   CHECK(ch.start(c, src, dst, 8, Dma::Priority::very_high));
   CHECK(regs.CTR1 == c.ctr1());
   CHECK(regs.CTR2 == c.ctr2());
   CHECK(regs.CBR1 == 8U);
   CHECK(regs.CSAR == addr(src));
   CHECK(regs.CDAR == addr(dst));
   CHECK(regs.CLLR == 0U);
   CHECK((regs.CCR & DMA_CCR_PRIO_Msk) == (3UL << DMA_CCR_PRIO_Pos));
   CHECK((regs.CCR & DMA_CCR_EN) != 0U);
   // Neighbour channels stay untouched.
   CHECK(gpdma.channel(2).CCR == 0U && gpdma.channel(4).CCR == 0U);
}

void testList() {
   FakeGpdma gpdma;
   Dma::Channel ch(gpdma.dma(), 0);
   auto &regs = gpdma.channel(0);
   regs.CSR = DMA_CSR_IDLEF;
   uint8_t src[16] = {};
   uint8_t dst[16] = {};
   Dma::Config c;
   c.completeEvent = Dma::CompleteEvent::LinkedListItem;

   Dma::LinkedListItem list[2];
   list[1] = Dma::LinkedListItem::make(c, src + 8, dst + 8, 8, nullptr);
   list[0] = Dma::LinkedListItem::make(c, src, dst, 8, &list[1]);
   CHECK(list[1].cllr == 0U); // end of list
   CHECK(list[0].cllr == ((addr(&list[1]) & 0xFFFCU) | Dma::LinkedListItem::updateAll));
   CHECK(list[0].cbr1 == 8U && list[0].csar == addr(src) && list[0].cdar == addr(dst));
   CHECK(list[0].ctr1 == c.ctr1() && list[0].ctr2 == c.ctr2());
   // The hardware reads the item as six words in register order.
   CHECK(sizeof(Dma::LinkedListItem) == 24U && offsetof(Dma::LinkedListItem, cllr) == 20U);

   CHECK(ch.startList(list[0]));
   CHECK(regs.CBR1 == 0U && regs.CTR2 == 0U); // empty LLI0
   CHECK(regs.CLBAR == (addr(&list[0]) & 0xFFFF0000U));
   CHECK(regs.CLLR == ((addr(&list[0]) & 0xFFFCU) | Dma::LinkedListItem::updateAll));
   CHECK((regs.CCR & DMA_CCR_EN) != 0U);

   // Circular list: the item points to itself.
   FakeGpdma other;
   Dma::Channel ch2(other.dma(), 1);
   other.channel(1).CSR = DMA_CSR_IDLEF;
   Dma::LinkedListItem loop = Dma::LinkedListItem::make(c, src, dst, 8, nullptr);
   loop.cllr = (addr(&loop) & 0xFFFCU) | Dma::LinkedListItem::updateAll;
   CHECK(ch2.startList(loop));
}

void testStatus() {
   FakeGpdma gpdma;
   Dma::Channel ch(gpdma.dma(), 7);
   auto &regs = gpdma.channel(7);
   CHECK(!ch.isIdle() && !ch.isEnabled());
   regs.CSR = DMA_CSR_IDLEF | DMA_CSR_TCF;
   regs.CBR1 = 0xFFFF1234U; // BRC and BNDT share the register
   CHECK(ch.isIdle() && ch.isTransferCompleteFlagSet() && !ch.isErrorFlagSet());
   CHECK(ch.remainingBytes() == 0x1234U);
   regs.CSR = DMA_CSR_USEF;
   CHECK(ch.isErrorFlagSet());
   ch.enableTransferCompleteInterrupt();
   ch.enableErrorInterrupts();
   CHECK((regs.CCR & (DMA_CCR_TCIE | DMA_CCR_DTEIE | DMA_CCR_USEIE | DMA_CCR_ULEIE)) ==
         (DMA_CCR_TCIE | DMA_CCR_DTEIE | DMA_CCR_USEIE | DMA_CCR_ULEIE));
   ch.disableAllInterrupts();
   CHECK(regs.CCR == 0U);
}

} // namespace

int main() {
   testConfig();
   testSingle();
   testList();
   testStatus();
   if(failures == 0) {
      std::printf("gpdma-h5: all tests passed\n");
   }
   return failures == 0 ? 0 : 1;
}
