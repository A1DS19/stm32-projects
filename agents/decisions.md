# Decisions

## 2026-08-15 — CLI-first toolchain over CubeIDE

All new work uses CubeCLT + headless CubeMX from the terminal; CubeIDE 2.0 stays installed but
only as a fallback GUI. Chosen for scriptability and because CubeIDE 2.0 dropped its embedded
scriptable CubeMX engine, making standalone CubeMX necessary anyway.

## 2026-08-15 — CMake as the generated toolchain

New projects generate with `project toolchain CMake` (presets Debug/Release, Ninja), not
Eclipse-managed builds or bare Makefiles. This is what CubeCLT and ST's VS Code flow target,
and it keeps projects buildable with nothing but the CLI. Legacy Eclipse projects are left
untouched rather than migrated wholesale.
