
# Directory Layout (WIP)
```py
.
├── .vscode/             # VSCode settings/config
├── Docs/
│   ├── DesignDoc.md     # This file (folder structure, task layout)
│   └── HardwareBoM.md   # Parts, cost, budget tracking
├── App/                 # Main C++ firmware location
├── Core/                # CubeMX managed C code
├── Drivers/             # Drivers for sensors
│   ├── GPS/
│   ├── IMU/
│   └── UltraSonic/
├── .gitignore
├── CMakeLists.txt
└── README.md
```
Generated with [markdowneditoronline.com](https://markdowneditoronline.com/tools/directory-tree-generator)

# RTOS Tasks (WIP)
| Task Name   | Description                               | Peripheral | Priority |
| :---------- | :---------------------------------------- | ---------: | :------: |
| Debug Print | Print debug messages over Debug UART      |  USART6 TX |   Low    |
| Read GPS    | Receive/parse NMEA messages from GPS UART |  USART1 RX | Moderate |
| App Main    | Print "Hello World" and blink LED         |        N/A |   Low    |

Generated with [markdowneditoronline.com](https://markdowneditoronline.com/tools/markdown-table-generator)
