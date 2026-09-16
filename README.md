# SerialTest

A small Windows serial debugging terminal written in C++ with wxWidgets. It is intended for ordinary COM-port debugging and works well for boards such as the Orange Pi AI Pro debug UART.

## Features

- Automatic COM-port discovery, including COM10 and above.
- Configurable baud rate, data bits, parity, stop bits, and flow control.
- Default serial settings: `115200 / 8 data bits / None parity / 1 stop bit / Flow control None`.
- Text and HEX receive display.
- Optional receive timestamps.
- Text and HEX sending.
- None / CR / LF / CRLF text line endings.
- Clear receive window and save displayed log to a UTF-8 text file.
- Background serial reader thread so the wxWidgets UI stays responsive.

## Repository layout

```text
serialTest/
├─ wxWidgets/          # downloaded locally; ignored by Git
├─ src/                # application and Win32 serial code
├─ tests/              # pure helper tests
├─ setup_wxwidgets.bat
├─ build.bat
└─ CMakeLists.txt
```

The repository does **not** contain the wxWidgets source tree. `setup_wxwidgets.bat` downloads wxWidgets `v3.2.8.1` into the local `wxWidgets/` folder.

## Requirements

- Windows 10 or Windows 11 x64.
- Git for Windows.
- CMake 3.20 or newer available in `PATH`.
- Visual Studio 2022 with **Desktop development with C++** installed.

## Quick start

Clone the repository and enter it:

```bat
git clone https://github.com/liguyano/serialTest.git
cd serialTest
```

If you are testing the feature branch before it is merged:

```bat
git switch feat/wx-serial-terminal
```

Download wxWidgets:

```bat
setup_wxwidgets.bat
```

This creates a complete local wxWidgets source tree at:

```text
serialTest\wxWidgets\
```

Build and run the tests:

```bat
build.bat
```

The Release executable is expected at:

```text
build\Release\SerialTest.exe
```

## Orange Pi AI Pro debug UART

For the Orange Pi AI Pro Micro USB debug serial connection, connect the board to Windows with a data-capable Micro USB cable. In Windows Device Manager, note the detected port, for example:

```text
USB Serial Device (COM3)
```

In SerialTest select:

```text
Port:         COM3 (or the port shown by Device Manager)
Baud:         115200
Data bits:    8
Parity:       None
Stop bits:    1
Flow control: None
```

Click **Open**. If the board is already running and nothing is printed, press Enter in the send box with `CRLF`, or power-cycle the board while the port is open to capture the complete boot log.

## Text and HEX modes

### Receive

- **Text** decodes incoming bytes as UTF-8 when possible and keeps readable ASCII control characters such as CR/LF/TAB.
- **HEX** displays bytes as uppercase two-digit values such as `48 65 6C 6C 6F`.
- **Timestamp** prefixes newly received blocks with the local time.

### Send

- **Text** converts the input to UTF-8 and can append None, CR, LF, or CRLF.
- **HEX** accepts either whitespace-separated or contiguous bytes:

```text
48 65 6C 6C 6F
```

or:

```text
48656C6C6F
```

Odd-length or non-hex input is rejected instead of sending partial data.

## Troubleshooting

### No COM port appears

1. Check Windows Device Manager under **Ports (COM & LPT)**.
2. Make sure the USB cable supports data and is not charge-only.
3. For the Orange Pi AI Pro, use the Micro USB debug-serial connector rather than the Type-C USB port.
4. Install the driver for the detected USB-to-serial device if Windows shows an unknown device.
5. Click **Refresh** in SerialTest after plugging the device in.

### `wxWidgets is missing`

Run:

```bat
setup_wxwidgets.bat
```

### CMake cannot find Visual Studio

Open **Visual Studio Installer**, modify Visual Studio 2022, and install **Desktop development with C++**. Then run `build.bat` again.

### Port opens in another program but not SerialTest

Only one program can normally own a COM port at a time. Close PuTTY, MobaXterm, Arduino Serial Monitor, or any other program using that COM port, then try again.
