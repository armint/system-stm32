/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2015 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if defined(__ARM_EABI__)

// ----------------------------------------------------------------------------

#include "cmsis-plus/cortexm/exception-handlers.h"
#include "trace_compat.h"
#include <cmsis_device.h>
#include <stdbool.h>
#include <string.h>

// ----------------------------------------------------------------------------

extern void __attribute__((noreturn, weak)) _start(void);


// ----------------------------------------------------------------------------

// Default exception handlers.
// Weak definitions, override them with similar
// handler routines defined in the application code.
//
// The ARCH_7M exception handlers are:
// 0x00 stack
// 0x04 Reset
// 0x08 NMI
// 0x0C HardFault
// 0x10 MemManage
// 0x14 BusFault
// 0x18 UsageFault
// 0x1C 0
// 0x20 0
// 0x24 0
// 0x28 0
// 0x2C SVC
// 0x30 DebugMon
// 0x34 0
// 0x38 PendSV
// 0x3C SysTick

// ----------------------------------------------------------------------------

// This function is not naked, and has a proper stack frame,
// to allow setting breakpoints at Reset_Handler.
void __attribute__((section(".after_vectors"), noreturn, weak)) Reset_Handler(void) {
   _start();
}

void __attribute__((section(".after_vectors"), weak)) NMI_Handler(void) {
#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

// ----------------------------------------------------------------------------


#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

// The values of BFAR and MMFAR remain unchanged if the BFARVALID or
// MMARVALID is set. However, if a new fault occurs during the
// execution of this fault handler, the value of the BFAR and MMFAR
// could potentially be erased. In order to ensure the fault addresses
// accessed are valid, the following procedure should be used:
// 1. Read BFAR/MMFAR.
// 2. Read CFSR to get BFARVALID or MMARVALID. If the value is 0, the
//    value of BFAR or MMFAR accessed can be invalid and can be discarded.
// 3. Optionally clear BFARVALID or MMARVALID.
// (See Joseph Yiu's book).

void dump_exception_stack(exception_stack_frame_t *frame, uint32_t cfsr, uint32_t mmfar, uint32_t bfar, uint32_t lr) {
   trace_printf("Stack frame:\n");
   trace_printf(" R0   = %08X\n", frame->r0);
   trace_printf(" R1   = %08X\n", frame->r1);
   trace_printf(" R2   = %08X\n", frame->r2);
   trace_printf(" R3   = %08X\n", frame->r3);
   trace_printf(" R12  = %08X\n", frame->r12);
   trace_printf(" LR   = %08X\n", frame->lr);
   trace_printf(" PC   = %08X\n", frame->pc);
   trace_printf(" PSR  = %08X\n", frame->psr);
   trace_printf("FSR/FAR:\n");
   trace_printf(" CFSR = %08X\n", cfsr);
   trace_printf(" HFSR = %08X\n", SCB->HFSR);
   trace_printf(" DFSR = %08X\n", SCB->DFSR);
   trace_printf(" AFSR = %08X\n", SCB->AFSR);

   if(cfsr & (1UL << 7)) {
      trace_printf(" MMFAR= %08X\n", mmfar);
   }
   if(cfsr & (1UL << 15)) {
      trace_printf(" BFAR = %08X\n", bfar);
   }
   trace_printf("Misc\n");
   trace_printf(" LR/EXC_RETURN = %08X\n", lr);
}

#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */

#if defined(__ARM_ARCH_6M__)

void dump_exception_stack(exception_stack_frame_t *frame, uint32_t lr) {
   trace_printf("Stack frame:\r\n");
   trace_printf(" R0  = %08X\r\n", frame->r0);
   trace_printf(" R1  = %08X\r\n", frame->r1);
   trace_printf(" R2  = %08X\r\n", frame->r2);
   trace_printf(" R3  = %08X\r\n", frame->r3);
   trace_printf(" R12 = %08X\r\n", frame->r12);
   trace_printf(" LR  = %08X\r\n", frame->lr);
   trace_printf(" PC  = %08X\r\n", frame->pc);
   trace_printf(" PSR = %08X\r\n", frame->psr);
   trace_printf("Misc\r\n");
   trace_printf(" LR/EXC_RETURN = %08X\r\n", lr);
}

#endif /* defined(__ARM_ARCH_6M__) */


// ----------------------------------------------------------------------------
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

// Hard Fault handler wrapper in assembly.
// Extract the location of the stack frame and pass it to the C
// handler as a pointer. Also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M3 and
// Cortex-M4 Processors, Third Edition, Chap. 12.8, page 402).

void __attribute__((section(".after_vectors"), weak, naked)) HardFault_Handler(void) {
   asm volatile(" tst lr,#4       \n"
                " ite eq          \n"
                " mrseq r0,msp    \n"
                " mrsne r0,psp    \n"
                " mov r1,lr       \n"
                // b.w has a range of +-16 MB and needs no literal pool.
                // With ldr r2,=X the pool must be at less than 4 KB, which
                // LTO can break when it puts more code in .after_vectors.
                " b.w HardFault_Handler_C"

                : /* Outputs */
                : /* Inputs */
                : /* Clobbers */
   );
}

void __attribute__((section(".after_vectors"), weak, used))
HardFault_Handler_C(exception_stack_frame_t *frame __attribute__((unused)), uint32_t lr __attribute__((unused))) {
   uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
   uint32_t bfar = SCB->BFAR;   // Bus Fault Address
   uint32_t cfsr = SCB->CFSR;   // Configurable Fault Status Registers


   trace_printf("[HardFault]\n");
   dump_exception_stack(frame, cfsr, mmfar, bfar, lr);

#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */

#if defined(__ARM_ARCH_6M__)

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M0
// First Edition, Chap. 12.8, page 402).

void __attribute__((section(".after_vectors"), weak, naked)) HardFault_Handler(void) {
   asm volatile(" movs r0,#4      \n"
                " mov r1,lr       \n"
                " tst r0,r1       \n"
                " beq 1f          \n"
                " mrs r0,psp      \n"
                " b   2f          \n"
                "1:               \n"
                " mrs r0,msp      \n"
                "2:"
                " mov r1,lr       \n"
                " ldr r2,=HardFault_Handler_C \n"
                " bx r2           \n"
                // ARMv6-M has no b.w: put the literal pool next to the ldr,
                // so that it is in range when LTO makes .after_vectors larger.
                " .ltorg"

                : /* Outputs */
                : /* Inputs */
                : /* Clobbers */
   );
}

void __attribute__((section(".after_vectors"), weak, used))
HardFault_Handler_C(exception_stack_frame_t *frame __attribute__((unused)), uint32_t lr __attribute__((unused))) {
   // There is no semihosting support for Cortex-M0, since on ARMv6-M
   // faults are fatal and it is not possible to return from the handler.

   // Emit diagnostics BEFORE any BKPT. On Cortex-M0+ the DCB registers are
   // not accessible from the CPU (see core_cm0plus.h), so we cannot detect
   // whether a debugger is attached. With no debugger, BKPT escalates into
   // another fault and locks up the core, which would suppress this output.
   trace_flush();
   trace_printf("[CHardFault]\n");
   dump_exception_stack(frame, lr);
   trace_flush();

#if defined(DEBUG)
   __BKPT(0);
#endif /* defined(DEBUG) */

   NVIC_SystemReset();

   while(true) {
      __NOP();
   }
}

#endif /* defined(__ARM_ARCH_6M__) */

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__((section(".after_vectors"), weak)) MemManage_Handler(void) {
#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

void __attribute__((section(".after_vectors"), weak, naked)) BusFault_Handler(void) {
   asm volatile(" tst lr,#4       \n"
                " ite eq          \n"
                " mrseq r0,msp    \n"
                " mrsne r0,psp    \n"
                " mov r1,lr       \n"
                // b.w has a range of +-16 MB and needs no literal pool.
                // With ldr r2,=X the pool must be at less than 4 KB, which
                // LTO can break when it puts more code in .after_vectors.
                " b.w BusFault_Handler_C"

                : /* Outputs */
                : /* Inputs */
                : /* Clobbers */
   );
}

void __attribute__((section(".after_vectors"), weak, used))
BusFault_Handler_C(exception_stack_frame_t *frame __attribute__((unused)), uint32_t lr __attribute__((unused))) {
   uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
   uint32_t bfar = SCB->BFAR;   // Bus Fault Address
   uint32_t cfsr = SCB->CFSR;   // Configurable Fault Status Registers

   trace_printf("[BusFault]\n");
   dump_exception_stack(frame, cfsr, mmfar, bfar, lr);

#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

void __attribute__((section(".after_vectors"), weak, naked)) UsageFault_Handler(void) {
   asm volatile(" tst lr,#4       \n"
                " ite eq          \n"
                " mrseq r0,msp    \n"
                " mrsne r0,psp    \n"
                " mov r1,lr       \n"
                // b.w has a range of +-16 MB and needs no literal pool.
                // With ldr r2,=X the pool must be at less than 4 KB, which
                // LTO can break when it puts more code in .after_vectors.
                " b.w UsageFault_Handler_C"

                : /* Outputs */
                : /* Inputs */
                : /* Clobbers */
   );
}

void __attribute__((section(".after_vectors"), weak, used))
UsageFault_Handler_C(exception_stack_frame_t *frame __attribute__((unused)), uint32_t lr __attribute__((unused))) {
   uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
   uint32_t bfar = SCB->BFAR;   // Bus Fault Address
   uint32_t cfsr = SCB->CFSR;   // Configurable Fault Status Registers

#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)

   if((cfsr & (1UL << 16)) != 0) // UNDEFINSTR
   {
      // For testing purposes, instead of BKPT use 'setend be'.
      if(is_semihosting(frame, AngelSWITestFaultOpCode)) {
         return;
      }
   }

#endif /* defined(OS_DEBUG_SEMIHOSTING_FAULTS) */

   trace_printf("[UsageFault]\n");
   dump_exception_stack(frame, cfsr, mmfar, bfar, lr);

#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

#endif

void __attribute__((section(".after_vectors"), weak)) SVC_Handler(void) {
#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__((section(".after_vectors"), weak)) DebugMon_Handler(void) {
#if defined(DEBUG)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */

void __attribute__((section(".after_vectors"), weak)) PendSV_Handler(void) {
#if defined(DEBUG)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
   if((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0) {
      __BKPT(0);
   }
#else
   __BKPT(0);
#endif /* defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) */
#endif /* defined(DEBUG) */

   while(true) {
      __NOP();
   }
}

void __attribute__((section(".after_vectors"), weak)) SysTick_Handler(void) {
   // DO NOT loop, just return.
   // Useful in case someone (like STM HAL) always enables SysTick.
}

// ----------------------------------------------------------------------------

#endif /* defined(__ARM_EABI__) */
