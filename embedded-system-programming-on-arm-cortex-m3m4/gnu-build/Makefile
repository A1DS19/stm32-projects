# gnu-build — the course's section 11 (slides 277-344): build a program
# for the Nucleo-L476RG with nothing but the GNU tools and this file.
# No IDE, no CMake: every command that runs is written here, in the
# order the slides teach them —
#
#   main.c --(preprocess)--> main.i --(compile)--> main.s --(assemble)--> main.o
#   *.o + stm32l476rg.ld --(link)--> build/final.elf (+ final.map)
#   final.elf --(STM32_Programmer_CLI or OpenOCD+GDB)--> the board
#
# Shared code is used straight from ../Src (uart2 printf backend, ST's
# newlib stubs, the FPU enable, the HardFault reporter) — reused, not
# copied. Targets: all (default), stages, analyze, flash, openocd, load,
# debug, serial, clean.

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

CUBE_FW := /home/dev/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2
SHARED  := ../Src
BUILD   := build

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
# __bss_start__ — the reason ST's script defines them). --specs=nosys:
# newlib's do-nothing stubs for any syscall ../Src/syscall.c does not
# provide (it provides all of them; the stubs are a safety net).
# -Map: the map file — where every symbol landed and why.
LDFLAGS := $(CPU) -T stm32l476rg.ld -nostartfiles --specs=nosys.specs \
           -Wl,-Map=$(BUILD)/final.map -Wl,--print-memory-usage

SRCS := main.c startup.c \
        $(SHARED)/uart2.c $(SHARED)/syscall.c $(SHARED)/sysmem.c \
        $(SHARED)/system.c $(SHARED)/faulthandler.c
OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o)))
vpath %.c . $(SHARED)

.PHONY: all stages analyze flash openocd load debug serial clean

all: $(BUILD)/final.elf
	$(SIZE) $<

# compile: one .c -> one relocatable .o (-c = stop before linking)
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# link: merge every .o's sections per the linker script, resolve symbols
$(BUILD)/final.elf: $(OBJS) stm32l476rg.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD):
	mkdir -p $@

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
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) -f openocd.cfg

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

clean:
	rm -rf $(BUILD)
