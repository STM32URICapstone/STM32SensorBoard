# Pre-requisites
## Hardware
See [HardwareBoM.md](HardwareBoM.md)

## Software
1. Install [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) (note: MX **not** MX2)
1. Install [STM32CubeProgrammer](https://www.st.com/content/st_com/en/stm32cubeprogrammer.html?tab=installer#stm32-software-downloads-container)
1. Install [VS Code](https://code.visualstudio.com/download)

# Setup
## Generate Project Files
This section needs to be completed only once unless you modify `STM32SensorBoard.ioc`

1. Download or clone this repository
1. Launch STM32CubeMX and click `File > Load Project`
1. Navigate to the repository folder and open `STM32SensorBoard.ioc`
1. Click `Project Manager` and ensure that `Toolchain Folder Location` is set to the repository folder.
1. Click `Generate Code`

## Build Project
1. Launch VS Code
1. Click `File > Open Folder`
1. Select the downloaded repository folder and open it
1. Once the window loads, install the recommended extensions (namely `STM32CubeIDE` and `cmake-tools`)
1. If prompted, disable the `clangd` extension for this workspace from the sidebar
1. After all extensions have installed, click `View > Command Palette`
1. Type `Developer: Reload Window` then enter
1. When prompted at the top, select `Debug` for configuration
1. After all extensions have loaded, click `View > Command Palette`
1. Type `CMake: Build` then enter
1. Verify `[build] Build finished with exit code 0` appears at the bottom of the Output console

## Programming
1. Connect the STM32 nucleo board to your PC
1. Launch STM32CubeProgrammer
1. In the `ST-Link Configuration` panel ensure:
    - Port: `SWD`
    - Mode: `Normal`
    - Reset mode: `Hardware reset`
1. Click `Connect` above the configuration
1. Click the download icon in the vertical navbar on the left 
1. Select `Browse` next to `File path`
1. Navigate to the repository folder then `build/Debug/STM32SensorBoard.elf`
1. Click `Start Programming`
1. Verify the progress bar at the bottom is at 100% and green

## Debugging (TODO)