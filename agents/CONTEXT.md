# stm32-projects — Context

Embedded firmware workspace for the STM32 Nucleo-L476RG, driven entirely from the CLI.

**CubeCLT**:
ST's official command-line toolset (compiler, programmer CLI, GDB server, CMake/Ninja) — the build/flash/debug backbone of this workspace.
_Avoid_: "the toolchain", "CubeIDE tools"

**Headless CubeMX**:
Standalone STM32CubeMX driven by a `-q <script>` command file to create and regenerate projects without the GUI.
_Avoid_: "CubeMX CLI mode", "quiet mode"

**Firmware pack**:
An STM32Cube HAL/CMSIS bundle (e.g. `STM32Cube_FW_L4`) that CubeMX copies from `~/STM32Cube/Repository/` into generated projects. Must be installed via `swmgr` before generation works.
_Avoid_: "SDK", "HAL download"

**cli-test**:
The reference CubeMX-generated CMake project; proves the generate → build → flash loop and blinks LD2 at 2.5 Hz as a known-good signal source.

**Legacy projects**:
`testing-target/` and `testing-host/` — Eclipse-managed CubeIDE projects with no standalone Makefile; buildable only through CubeIDE until migrated.
_Avoid_: treating them as part of the CMake flow

**LA2016**:
The Kingst 16-channel logic analyzer on this bench (KingstVIS software); validates firmware behavior independently of the code.
