#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern void initialise_monitor_handles(void);

// Cortex-M cycle-counting registers. See the ARM v7-m Architecture Reference
// Manual for details.
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000)
#define DWT_CTRL_CYCCNTENA (1u << 0)

void enable_cycle_counter(void) { DWT_CTRL |= DWT_CTRL_CYCCNTENA; }
uint32_t read_cycle_counter(void) { return DWT_CYCCNT; }

void test_memcpy(void) {
  struct test_struct {
    char data[4096];
  };
  // instantiate 2 structs. for our purposes, we don't care what data is in
  // there. set them to `volatile` so the compiler won't optimize away what we
  // do with them
  volatile struct test_struct dest, source;

  enable_cycle_counter(); // << Enable Cycle Counter

  // run through powers-of-two memcpy's, printing stats for each test
  for (size_t len = 1; len <= sizeof(dest); len <<= 1) {
    uint32_t start = read_cycle_counter(); // << Start count
    memcpy((void *)&dest, (void *)&source, len);
    uint32_t stop = read_cycle_counter(); // << Stop count

    // print out the cycles consumed
    printf("len = %lu, cyccnt = %lu, cycles/byte = %0.3f\n", (uint32_t)len, stop - start,
           (float)(stop - start) / (float)len);
  }
}

int main(void) {
  initialise_monitor_handles();

  // line buffering on stdout
  setvbuf(stdout, NULL, _IOLBF, 0);

  printf("🦄 Hello there!\n");

  test_memcpy();

  while (1) {
  };

  return 0;
}

///
/// The below code is used to bootstrap our main
///

// These symbols are defined by the linker
extern uint32_t __etext, __data_start__, __data_end__, __bss_start__,
    __bss_end__, __stack;

static void prv_cinit(void) {
  // Initialize data and bss
  // Copy the data segment initializers from flash to SRAM
  for (uint32_t *dst = &__data_start__, *src = &__etext; dst < &__data_end__;) {
    *(dst++) = *(src++);
  }

  // Zero fill the bss segment.
  for (uint32_t *dst = &__bss_start__;
       (uintptr_t)dst < (uintptr_t)&__bss_end__;) {
    *(dst++) = 0;
  }
}

__attribute__((noreturn)) void Reset_Handler(void) {
  // __ARM_FP is defined by the compiler if -mfloat-abi=hard is set
  #if defined(__ARM_FP)
  // enable floating-point access; some instructions emitted at -O3 will make
  // use of the FP co-processor, eg vldr.64
#define CPACR (*(volatile uint32_t *)0xE000ED88)
  CPACR |= ((3UL << 10 * 2) | /* set CP10 Full Access */
            (3UL << 11 * 2)); /* set CP11 Full Access */
  #endif

  prv_cinit();

  // Call the application's entry point.
  (void)main();

  // shouldn't return
  while (1) {
  };
}

__attribute__((weak)) void HardFault_Handler(void) {
  __asm__("bkpt 92");
  while (1) {
  };
}

// A minimal vector table for a Cortex M.
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
    (void *)(&__stack), // initial stack pointer
    Reset_Handler,
    0,
    HardFault_Handler,
};
