CUBECLT  := /opt/st/stm32cubeclt_1.22.0
PROJECT  := button-exti
BUILD    := build/Debug
ELF      := $(BUILD)/$(PROJECT).elf
GDB_PORT := 61234

export PATH := $(CUBECLT)/CMake/bin:$(CUBECLT)/Ninja/bin:$(CUBECLT)/GNU-tools-for-STM32/bin:$(CUBECLT)/STM32CubeProgrammer/bin:$(CUBECLT)/STLink-gdb-server/bin:$(PATH)

.PHONY: build flash serial debug gdb-server size symbols disasm reset clean

build: $(BUILD)/build.ninja
	cmake --build $(BUILD)

$(BUILD)/build.ninja:
	cmake --preset Debug

flash: build
	STM32_Programmer_CLI -c port=SWD -w $(ELF) -v -rst

# Resolve the board's VCP by USB identity — never assume /dev/ttyACM0
serial:
	@p=$$(for d in /dev/ttyACM*; do udevadm info -q property -n $$d 2>/dev/null | grep -q STM32_STLink && echo $$d && break; done); \
	[ -n "$$p" ] || { echo "no ST-LINK VCP found — board plugged in?"; exit 1; }; \
	tio $$p

# -d (SWD) is required: without it the server exits 5 with "Unknown MCU found on target"
gdb-server:
	ST-LINK_gdbserver -p $(GDB_PORT) -d -cp $(CUBECLT)/STM32CubeProgrammer/bin

debug: build
	arm-none-eabi-gdb $(ELF) -ex 'target remote :$(GDB_PORT)'

size: build
	arm-none-eabi-size $(ELF)

symbols: build
	arm-none-eabi-nm --print-size --size-sort -C $(ELF) | tail -25

disasm: build
	arm-none-eabi-objdump -d -S $(ELF) | less

reset:
	STM32_Programmer_CLI -c port=SWD -rst

clean:
	rm -rf build
