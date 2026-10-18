// Host test for hw::Uid: the byte order of read() and its agreement with
// word(). The 32-bit access width cannot be tested on the host.

#include "peripherals/Uid.h"

#include <sys/mman.h>

#include <array>
#include <cstdio>

namespace {

int failures = 0;

#define CHECK(cond)                                                  \
   do {                                                              \
      if(!(cond)) {                                                  \
         std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
         ++failures;                                                 \
      }                                                              \
   } while(0)

} // namespace

int main() {
   void *page = mmap(reinterpret_cast<void *>(UID_BASE & ~0xFFFUL), 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
   if(page != reinterpret_cast<void *>(UID_BASE & ~0xFFFUL)) {
      std::printf("cannot map the fake UID at 0x%lx\n", UID_BASE);
      return 2;
   }
   auto *words = reinterpret_cast<volatile uint32_t *>(UID_BASE);
   words[0] = 0x04030201u;
   words[1] = 0x08070605u;
   words[2] = 0x0C0B0A09u;

   std::array<uint8_t, hw::Uid::SizeBytes> uid{};
   hw::Uid::read(uid);
   for(size_t i = 0; i < uid.size(); ++i) {
      CHECK(uid[i] == i + 1u);
   }
   CHECK(hw::Uid::word(0) == 0x04030201u);
   CHECK(hw::Uid::word(2) == 0x0C0B0A09u);
   CHECK(hw::Uid::word(3) == 0u);
   CHECK(hw::Uid::hash() == (0x04030201u ^ 0x08070605u ^ 0x0C0B0A09u));

   if(failures == 0) {
      std::printf("OK\n");
   }
   return failures == 0 ? 0 : 1;
}
