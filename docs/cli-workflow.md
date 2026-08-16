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
~/STM32CubeMX/STM32CubeMX -q gen.txt
```

Caveats:
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
ST-LINK_gdbserver -p 61234 -cp /opt/st/stm32cubeclt_1.22.0/STM32CubeProgrammer/bin &
arm-none-eabi-gdb build/Debug/my-app.elf -ex 'target remote :61234'
```

Then the usual gdb: `break main`, `continue`, `next`, `print var`, `monitor reset`.

## 6. Change hardware config (pins, peripherals, clocks)

Open the project's `.ioc` in CubeMX (`~/STM32CubeMX/STM32CubeMX my-app/my-app.ioc`), make the
change, regenerate. USER CODE blocks survive. This is the one recurring GUI touchpoint — the
pinout-conflict and clock-tree assistant is what CubeMX is for.

## 7. Serial console

The ST-LINK exposes USART2 (PA2/PA3) as a virtual COM port on the host:

```sh
tio /dev/ttyACM0              # or: screen /dev/ttyACM0 115200
```

Note: PA2/PA3 route to the ST-LINK, not to the Arduino D0/D1 pins — tap them on the Morpho
header when probing with the logic analyzer.
