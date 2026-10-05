# Repository Guidelines

## Project Structure & Module Organization

This is a TI Code Composer Studio (CCS) bare-metal example for the LP-MSPM0G3507 LaunchPad. `empty.c` contains the application loop and calls the SysConfig-generated DriverLib initialization. `empty.syscfg` defines the device and pin assignments, including the LED on PB14. CCS project settings live in `.project`, `.cproject`, and `.ccsproject`; `targetConfigs/MSPM0G3507.ccxml` defines the debug target. `Debug/` contains generated configuration, build files, and output artifacts. `README.md` documents the board and example behavior. There is currently no separate test or asset directory.

## Build, Run & Development Commands

- Import this directory as an existing project in CCS, then select **Project > Build Project** to regenerate SysConfig outputs and build the Debug configuration.
- Use **Run > Debug** in CCS with the LaunchPad connected to load and run the resulting `.out` file. Confirm the LED on PB14 toggles.
- If CCS has already generated the build files and the configured TI tools are installed, run `make -C Debug all` to rebuild or `make -C Debug clean` to remove build outputs. The generated Makefile uses Windows `cmd.exe` and machine-specific tool paths; prefer CCS when those paths differ.

## Coding Style & Configuration

Use four spaces for C indentation and braces around control-flow bodies, matching `empty.c`. Keep peripheral names consistent with SysConfig symbols such as `GPIO_GRP_LED_PORT`. Put application logic in source files rather than generated files under `Debug/`. Change pin and peripheral settings through SysConfig tooling; `empty.syscfg` explicitly warns against direct manual edits. Preserve the TI copyright and license notice when editing `empty.c`. No formatter or linter is configured in this repository.

## Testing Guidelines

No automated test framework or coverage target is configured. For firmware changes, build the Debug configuration, flash the board, and check the affected behavior on hardware. For the current example, verify the LED toggles and the CCS build reports no errors. Describe the hardware used and the observed result in the pull request.

## Commits & Pull Requests

This directory has no Git history, so no existing commit convention can be inferred. Use short, imperative commit subjects, for example `Configure PB14 LED output`. Pull requests should explain the change, identify any pin or SysConfig changes, include build and hardware verification results, and link a relevant issue when one exists. Add screenshots only when they help review a CCS or SysConfig UI change.
