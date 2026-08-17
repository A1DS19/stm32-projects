# CLI workflow — Nucleo-L476RG

The full project lifecycle from the terminal. Step 1 happens once per project; step 6 only when
the hardware configuration changes.

## 1. Create (once per project)

Write a CubeMX script, e.g. `gen.txt`:

```
loadboard NUCLEO-L476RG allmodes
project name my-app
project toolchain CMake
project path /home/dev/projects/stm32-projects/my-app
project generate
exit
```

Run it:

```sh
~/STM32CubeMX/STM32CubeMX -q "$PWD/gen.txt"
```

Caveats:
- The script path must be absolute: CubeMX chdirs to its install dir on startup, so a
  relative path resolves against `~/STM32CubeMX/`, and on a missing script `-q` doesn't
  exit — it hangs as an idle interactive session (FileNotFoundException buried in the log).
- The MCU family's firmware pack must already be installed, or `project generate` silently
  writes only the `.ioc`: `swmgr install stm32cube_l4_1.18.2 ask` (once per family).
- `-q` mode still pops GUI dialogs when a display is present (e.g. Board Project Options on
  `loadboard`) — click OK to unblock it.

## 2. Build

```sh
cd my-app
cmake --preset Debug          # configure, once
cmake --build build/Debug     # rebuild after every edit
```

New login shells have CubeCLT on PATH; scripts must `source /etc/profile.d/cubeclt-bin-path_1.22.0.sh`.
For editor LSP support: configure with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and symlink
`build/Debug/compile_commands.json` to the project root.

## 3. Edit

User code goes only between `/* USER CODE BEGIN/END */` markers in `Src/` — everything outside
them is CubeMX-owned and rewritten on regeneration.

## 4. Flash

```sh
STM32_Programmer_CLI -c port=SWD -w build/Debug/my-app.elf -v -rst
```

Writes, verifies, resets; the board runs the new firmware immediately.

## 5. Debug

```sh
ST-LINK_gdbserver -p 61234 -d -cp /opt/st/stm32cubeclt_1.22.0/STM32CubeProgrammer/bin &
arm-none-eabi-gdb build/Debug/my-app.elf -ex 'target remote :61234'
```

`-d` selects SWD — without it the server tries JTAG and dies with "Unknown MCU found on target".

Then the usual gdb: `break main`, `continue`, `next` (over) / `step` (into), `info locals`,
`print var`, `monitor reset` to restart the program; `layout src` gives a source-level
stepping view in the terminal.

While halted at a breakpoint the firmware is frozen — serial output pauses and
`HAL_Delay` timing stretches; that's the debugger, not a hang. After a debug session,
`STM32_Programmer_CLI -c port=SWD -rst` gets the board running standalone again.

## 6. Change hardware config (pins, peripherals, clocks)

Open the project's `.ioc` in CubeMX (`~/STM32CubeMX/STM32CubeMX my-app/my-app.ioc`), make the
change, regenerate. USER CODE blocks survive. This is the one recurring GUI touchpoint — the
pinout-conflict and clock-tree assistant is what CubeMX is for.

## 7. Serial console

The ST-LINK exposes USART2 (PA2/PA3) as a virtual COM port on the host:

```sh
tio /dev/ttyACM1              # or: screen /dev/ttyACM1 115200
```

Don't assume the port number — other CDC devices (e.g. the Turing USB monitor) claim
`/dev/ttyACM0` first on this machine. Identify the board's port with:

```sh
for d in /dev/ttyACM*; do udevadm info -q property -n $d | grep -q STM32_STLink && echo $d; done
```

Note: PA2/PA3 route to the ST-LINK, not to the Arduino D0/D1 pins — tap them on the Morpho
header when probing with the logic analyzer.

## 8. Inspecting binaries (disassembly, ELF, size)

All from CubeCLT's binutils, run against `build/Debug/<name>.elf`:

```sh
arm-none-eabi-objdump -d -S <elf> | less        # disassembly, C source interleaved (-g builds)
arm-none-eabi-objdump -d -S --disassemble=main <elf>   # a single function
arm-none-eabi-readelf -h <elf>                  # header: entry point (= Reset_Handler, not main)
arm-none-eabi-readelf -S <elf>                  # section table: .text/.data/.bss addresses
arm-none-eabi-size <elf>                        # flash/RAM totals per section
arm-none-eabi-nm --print-size --size-sort -C <elf>     # symbols by size — "where did my flash go"
arm-none-eabi-addr2line -e <elf> -f -C 0x08002dbe      # address -> function + source line
```

- **Crash triage:** on a HardFault, take the stacked PC (from the fault handler or gdb) and
  feed it to `addr2line` — it names the exact source line.
- **The map file:** the linker writes `build/Debug/<name>.map` — every symbol's placement in
  FLASH/RAM and which object pulled it in. Plain text, just search it.
- **Live disassembly:** inside gdb, `disassemble main` or `x/10i $pc` shows instructions with
  the real register state at hand.
- Nano-libc quirk worth knowing: `printf` compiles to `iprintf` (integer-only) unless float
  support is linked — visible in the disassembly, and why `%f` silently misbehaves by default.
