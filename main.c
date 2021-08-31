#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern void initialise_monitor_handles(void);

int main(void);

// Following symbols are defined by the linker.
// Start address for the initialization values of the .data section.
extern uint32_t __etext;
// Start address for the .data section
extern uint32_t __data_start__;
// End address for the .data section
extern uint32_t __data_end__;
// Start address for the .bss section
extern uint32_t __bss_start__;
// End address for the .bss section
extern uint32_t __bss_end__;
// End address for stack
extern uint32_t __stack;

// Prevent inlining to avoid persisting any stack allocations
__attribute__((noinline)) static void prv_cinit(void) {
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

// Cortex-M cycle-counting registers. See the ARM v7-m Architecture Reference
// Manual for details.
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000)
#define DWT_CTRL_CYCCNTENA (1u << 0)

void enable_cycle_counter(void) { DWT_CTRL |= DWT_CTRL_CYCCNTENA; }
uint32_t read_cycle_counter(void) { return DWT_CYCCNT; }

void test_memcpy(void) {
  struct _128_bytes {
    char data[128];
  };
  // instantiate 2 structs. for our purposes, we don't care what data is in
  // there. set them to `volatile` so the compiler won't optimize away what we
  // do with them
  volatile struct _128_bytes dest, source;

  enable_cycle_counter(); // << Enable Cycle Counter

  uint32_t start = read_cycle_counter(); // << Start count
  memcpy((void *)&dest, (void *)&source, sizeof(dest));
  uint32_t stop = read_cycle_counter(); // << Stop count

  // print out the cycles consumed
  printf("memcpy cyccnt = %lu\n", stop - start);
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
