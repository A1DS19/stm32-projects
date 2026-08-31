# gnu-build — the course's sections 11 and 12 (slides 277-378): build a
# program for the Nucleo-L476RG with nothing but the GNU tools and this
# file. No IDE, no CMake: every command that runs is written here, in
# the order the slides teach them —
#
#   main.c --(preprocess)--> main.i --(compile)--> main.s --(assemble)--> main.o
#   *.o + stm32l476rg.ld --(link)--> build/final.elf (+ final.map)
#   final.elf --(STM32_Programmer_CLI or OpenOCD+GDB)--> the board
#
# Section 12 links the SAME program three more ways (newlib-nano, dead
# code removed, semihosting) and drives OpenOCD from scripts. Shared code
# is used straight from ../Src (uart2 printf backend, ST's newlib stubs,
# the FPU enable, the HardFault reporter) — reused, not copied.
#
# Targets — section 11: all (default), stages, analyze, flash, openocd,
# load, debug, serial, clean. Section 12: nano, gc, semi, compare,
# analyze-gc, inspect, run-semi.

CUBECLT    := /opt/st/stm32cubeclt_1.22.0
TOOLS      := $(CUBECLT)/GNU-tools-for-STM32/bin
CC         := $(TOOLS)/arm-none-eabi-gcc
OBJDUMP    := $(TOOLS)/arm-none-eabi-objdump
READELF    := $(TOOLS)/arm-none-eabi-readelf
NM         := $(TOOLS)/arm-none-eabi-nm
SIZE       := $(TOOLS)/arm-none-eabi-size
GDB        := $(TOOLS)/arm-none-eabi-gdb
PROGRAMMER := $(CUBECLT)/STM32CubeProgrammer/bin/STM32_Programmer_CLI

# OpenOCD: not installed system-wide, but CubeIDE 2.0 bundles ST's build
# and the board/target scripts it needs.
CUBEIDE_PLUGINS := /opt/st/stm32cubeide_2.0.0/plugins
OPENOCD         := $(wildcard $(CUBEIDE_PLUGINS)/com.st.stm32cube.ide.mcu.externaltools.openocd.linux64_*/tools/bin/openocd)
OPENOCD_SCRIPTS := $(wildcard $(CUBEIDE_PLUGINS)/com.st.stm32cube.ide.mcu.debug.openocd_*/resources/openocd/st_scripts)
OPENOCD_RUN     := $(OPENOCD) -s $(OPENOCD_SCRIPTS) -f openocd.cfg

CUBE_FW := /home/dev/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2
SHARED  := ../Src
BUILD   := build
# section 12 variants, see the end of the file
NANO_DIR := $(BUILD)/nano
GC_DIR   := $(BUILD)/gc
SEMI_DIR := $(BUILD)/semi

# The processor: Cortex-M4, Thumb instructions only (there is no ARM
# mode on Cortex-M), the single-precision FPU, hard-float calling
# convention (float arguments travel in FPU registers — the same choice
# ST's CMake makes, so the shared code links unchanged).
CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# -O0 -g3: no optimization, full debug info — the code you read is the
# code that runs. No -ffunction-sections here on purpose: each .o then
# shows exactly the four sections of the slide (.text .data .bss .rodata).
CFLAGS := $(CPU) -std=gnu11 -O0 -g3 -Wall -Wextra -Wno-unused-parameter -DSTM32L476xx \
          -I../Inc \
          -I$(CUBE_FW)/Drivers/CMSIS/Device/ST/STM32L4xx/Include \
          -I$(CUBE_FW)/Drivers/CMSIS/Include

# -T: our linker script. -nostartfiles: gcc would otherwise add newlib's
# crt0.o, whose _start does Reset_Handler's job (and wants symbols like
# __bss_start__ — the reason ST's script defines them). Each link below
# adds its own --specs (which C library and which syscall stubs) and its
# own -Map: the map file says where every symbol landed and why.
LDFLAGS := $(CPU) -T stm32l476rg.ld -nostartfiles -Wl,--print-memory-usage

SRCS := main.c startup.c \
        $(SHARED)/uart2.c $(SHARED)/syscall.c $(SHARED)/sysmem.c \
        $(SHARED)/system.c $(SHARED)/faulthandler.c
OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o)))
vpath %.c . $(SHARED)

.PHONY: all stages analyze flash openocd load debug serial clean \
        nano gc semi compare analyze-gc inspect run-semi

all: $(BUILD)/final.elf
	$(SIZE) $<

# compile: one .c -> one relocatable .o (-c = stop before linking)
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# link: merge every .o's sections per the linker script, resolve symbols.
# --specs=nosys.specs: newlib's do-nothing stubs for any syscall
# ../Src/syscall.c does not provide (it provides all of them; a safety net).
$(BUILD)/final.elf: $(OBJS) stm32l476rg.ld
	$(CC) $(LDFLAGS) --specs=nosys.specs -Wl,-Map=$(BUILD)/final.map $(OBJS) -o $@

# the three stages of the slide, one file at a time
stages: | $(BUILD)
	$(CC) $(CFLAGS) -E main.c -o $(BUILD)/main.i
	$(CC) $(CFLAGS) -S main.c -o $(BUILD)/main.s
	$(CC) $(CFLAGS) -c main.c -o $(BUILD)/main.o
	wc -l main.c $(BUILD)/main.i $(BUILD)/main.s

# read the objects and the executable the way the slides do
analyze: $(BUILD)/final.elf
	@echo "== main.o: sections of ONE relocatable object (addresses all 0 — nothing placed yet) =="
	$(OBJDUMP) -h $(BUILD)/main.o | grep -E "Idx|\.text|\.data|\.bss|\.rodata"
	@echo
	@echo "== startup.o: same, plus our own .isr_vector section =="
	$(OBJDUMP) -h $(BUILD)/startup.o | grep -E "Idx|isr_vector|\.text|\.data|\.bss|\.rodata"
	@echo
	@echo "== relocatable means: calls into other files carry a relocation, not an address =="
	$(OBJDUMP) -d -r $(BUILD)/main.o | grep -B1 "R_ARM_THM_CALL" | head -6
	@echo
	@echo "== final.elf: entry point (ENTRY in the script) and LOAD segments =="
	$(READELF) -h $(BUILD)/final.elf | grep "Entry"
	$(READELF) -l $(BUILD)/final.elf | grep -E "LOAD|VirtAddr"
	@echo
	@echo "== output sections, addresses assigned by OUR linker script =="
	$(READELF) -S -W $(BUILD)/final.elf | grep -E "Name|\.text|\.data|\.bss|init_array|exidx"
	@echo
	@echo "== the symbols Reset_Handler walks =="
	$(NM) $(BUILD)/final.elf | grep -E " (_estack|_sidata|_sdata|_edata|_sbss|_ebss|_end|_etext|vectors)$$"
	@echo
	@echo "== biggest symbols (printf's engine dominates a 4 KB program) =="
	$(NM) --print-size --size-sort $(BUILD)/final.elf | tail -6

# program + verify + reset over SWD, the workspace's usual path
flash: $(BUILD)/final.elf
	$(PROGRAMMER) -c port=SWD -w $< -v -rst

# the course's path: an OpenOCD server, then a GDB client that loads the ELF
openocd:
	$(OPENOCD_RUN)

load: $(BUILD)/final.elf
	$(GDB) -batch $< -ex 'target extended-remote :3333' \
	       -ex 'monitor reset halt' -ex 'load' -ex 'monitor reset run'

debug: $(BUILD)/final.elf
	$(GDB) $< -ex 'target extended-remote :3333' \
	       -ex 'monitor reset halt' -ex 'load' -ex 'break main' -ex 'continue'

serial:
	@p=$$(for d in /dev/ttyACM*; do udevadm info -q property -n $$d 2>/dev/null | grep -q STM32_STLink && echo $$d && break; done); \
	[ -n "$$p" ] || { echo "no ST-LINK VCP found — board plugged in?"; exit 1; }; \
	tio $$p

$(BUILD) $(NANO_DIR) $(GC_DIR) $(SEMI_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

# ---------------------------------------------------------------------
# Section 12: the same program, three more links

NANO_OBJS := $(addprefix $(NANO_DIR)/,$(notdir $(SRCS:.c=.o)))
GC_OBJS   := $(addprefix $(GC_DIR)/,$(notdir $(SRCS:.c=.o)))
# semihosting: librdimon supplies every syscall stub, so ST's syscall.c
# and the UART driver stay out — two _write()s would collide at link time
SEMI_SRCS := $(filter-out $(SHARED)/syscall.c $(SHARED)/uart2.c,$(SRCS))
SEMI_OBJS := $(addprefix $(SEMI_DIR)/,$(notdir $(SEMI_SRCS:.c=.o)))

# newlib-nano: --specs=nano.specs is a COMPILE flag too (it switches the
# headers to a smaller struct _reent), and at link time swaps -lc for
# -lc_nano: an integer-only printf engine and a bins-free malloc.
# Floats in printf would need -u _printf_float (ST's CMake passes it).
nano: $(NANO_DIR)/final-nano.elf
	$(SIZE) $<

$(NANO_DIR)/%.o: %.c | $(NANO_DIR)
	$(CC) $(CFLAGS) --specs=nano.specs -c $< -o $@

$(NANO_DIR)/final-nano.elf: $(NANO_OBJS) stm32l476rg.ld
	$(CC) $(LDFLAGS) --specs=nano.specs --specs=nosys.specs -Wl,-Map=$(NANO_DIR)/final-nano.map $(NANO_OBJS) -o $@

# dead code removal: -ffunction-sections/-fdata-sections give every
# function and variable its own input section, --gc-sections drops the
# ones nothing references. The vector table survives only because the
# linker script says KEEP.
gc: $(GC_DIR)/final-gc.elf
	$(SIZE) $<

$(GC_DIR)/%.o: %.c | $(GC_DIR)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -c $< -o $@

$(GC_DIR)/final-gc.elf: $(GC_OBJS) stm32l476rg.ld
	$(CC) $(LDFLAGS) --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=$(GC_DIR)/final-gc.map $(GC_OBJS) -o $@

# semihosting: --specs=rdimon.specs links librdimon, whose _write()/_read()
# are BKPT 0xAB traps a debugger services (OpenOCD: `arm semihosting
# enable`). main.c calls initialise_monitor_handles() instead of uart2_init().
semi: $(SEMI_DIR)/final-semi.elf
	$(SIZE) $<

$(SEMI_DIR)/%.o: %.c | $(SEMI_DIR)
	$(CC) $(CFLAGS) -DSEMIHOSTING -c $< -o $@

$(SEMI_DIR)/final-semi.elf: $(SEMI_OBJS) stm32l476rg.ld
	$(CC) $(LDFLAGS) --specs=rdimon.specs -Wl,-Map=$(SEMI_DIR)/final-semi.map $(SEMI_OBJS) -o $@

# the numbers are the lesson
compare: $(BUILD)/final.elf $(NANO_DIR)/final-nano.elf $(GC_DIR)/final-gc.elf $(SEMI_DIR)/final-semi.elf
	@echo "same program, four links (bytes):"
	$(SIZE) $^

analyze-gc: $(GC_DIR)/final-gc.elf
	@echo "== input sections the gc link threw away (from the map; our bait among them) =="
	sed -n '/^Discarded input sections/,/^Memory Configuration/p' $(GC_DIR)/final-gc.map | grep -E "unused|\.text\.|\.rodata\." | head -12
	@echo
	@echo "== the vector table survived — KEEP in the linker script =="
	$(NM) $(GC_DIR)/final-gc.elf | grep " vectors$$"

# the debugger reads and writes our memory through the AHB-AP: the vector
# words, then garbage planted where global_init lives — and after
# Reset_Handler's copy loop has run, 42 is back
inspect: $(BUILD)/final.elf
	$(OPENOCD_RUN) -c "init" -c "reset halt" -c "program $< verify" -c "reset halt" \
	  -c "echo {-- vector words at 0x08000000, read through the debug port:}" -c "mdw 0x08000000 4" \
	  -c "echo {-- global_init before Reset_Handler runs: plant garbage there}" \
	  -c "mww 0x20000000 0xDEADBEEF" -c "mdw 0x20000000 1" \
	  -c "resume" -c "sleep 500" -c "halt" \
	  -c "echo {-- after Reset_Handler's copy loop:}" -c "mdw 0x20000000 1" \
	  -c "shutdown"

# semihosting, twice: first with nobody listening (the BKPT stops the core
# — what a release build with semihosting left on does in the field),
# then with OpenOCD answering — the lesson text arrives on THIS console
run-semi: $(SEMI_DIR)/final-semi.elf
	$(OPENOCD_RUN) -c "init" -c "reset halt" -c "program $< verify" \
	  -c "echo {-- semihosting OFF: run, then look where the core stopped}" \
	  -c "reset run" -c "sleep 1000" -c "halt" -c "reg pc" \
	  -c "echo {-- semihosting ON: same program, the debugger now answers each BKPT}" \
	  -c "arm semihosting enable" -c "reset run" -c "sleep 12000" -c "shutdown"
