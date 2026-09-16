# wxWidgets Windows Serial Terminal Design

## Goal

Build a Windows-only serial debugging terminal in C++ using wxWidgets for the GUI and the Win32 serial API for COM-port access. The primary target is debugging devices such as Orange Pi AI Pro over a USB serial adapter, including the current `COM3` / `115200` use case.

## Repository Layout

The repository stores only the application source, build scripts, documentation, and tests. wxWidgets itself is not committed to GitHub. A helper script downloads the wxWidgets source tree into a local `wxWidgets/` directory so the local layout mirrors a normal wxWidgets checkout.

```text
serialTest/
├─ wxWidgets/                 # generated locally, ignored by Git
├─ src/
│  ├─ App.cpp
│  ├─ App.h
│  ├─ MainFrame.cpp
│  ├─ MainFrame.h
│  ├─ SerialPort.cpp
│  └─ SerialPort.h
├─ tests/
│  └─ SerialHelpersTests.cpp
├─ resources/
├─ setup_wxwidgets.bat
├─ build.bat
├─ CMakeLists.txt
├─ .gitignore
└─ README.md
```

`wxWidgets/` must remain isolated from application code and must be ignored by Git.

## wxWidgets Bootstrap

`setup_wxwidgets.bat` will:

1. Verify that Git is available.
2. Skip downloading when `wxWidgets/CMakeLists.txt` already exists.
3. Clone the official wxWidgets repository recursively into `wxWidgets/` so bundled third-party submodules are present.
4. Checkout a fixed stable wxWidgets release tag rather than tracking the moving default branch.
5. Print clear success and error messages.

The build must not require a machine-wide wxWidgets installation.

## Build System

The project uses CMake and Visual Studio's MSVC toolchain on Windows.

`CMakeLists.txt` will add the local wxWidgets checkout with `add_subdirectory(wxWidgets ...)` and build the application from `src/`. The GUI target is a Windows executable and links against the wxWidgets core/base components and required Windows libraries.

`build.bat` will:

1. Check that `wxWidgets/` exists and instruct the user to run `setup_wxwidgets.bat` if it does not.
2. Configure a 64-bit Visual Studio build into a separate `build/` directory.
3. Build the Release configuration.
4. Print the resulting executable path.

`build/` is ignored by Git.

## Application Architecture

### App

`App` is the wxWidgets application entry point. It initializes wxWidgets, creates `MainFrame`, and hands control to the event loop.

### MainFrame

`MainFrame` owns the user interface and orchestrates serial operations. It does not directly call Win32 serial APIs; it communicates only with `SerialPort`.

The main window contains:

- Port selector with refresh button.
- Baud-rate selector with `115200` as the default.
- Data bits selector.
- Parity selector.
- Stop bits selector.
- Flow-control selector, default `None`.
- Open/Close button.
- Connection-state indicator.
- Large receive console.
- Text/HEX receive display selector.
- Timestamp toggle.
- Send input field.
- Text/HEX send selector.
- Send button.
- Optional CR, LF, and CRLF line-ending choices for text mode.
- Clear receive window button.
- Save receive log button.

Controls that would change active serial settings are disabled while the port is open.

### SerialPort

`SerialPort` encapsulates all Win32 COM-port interaction.

Responsibilities:

- Enumerate available COM ports.
- Open a port using `CreateFileW` and the `\\.\COMx` form so COM10+ works.
- Configure baud rate, data bits, parity, stop bits, and flow control using `DCB`.
- Configure read/write timeouts.
- Run a dedicated reader thread so the GUI remains responsive.
- Deliver received byte blocks through a callback.
- Send arbitrary byte vectors.
- Stop the reader thread and close the handle safely.
- Expose clear error messages derived from Windows error codes.

The serial worker thread must never modify wxWidgets controls directly. `MainFrame` marshals receive notifications onto the GUI thread using wxWidgets event posting / `CallAfter`.

## Serial Defaults

Default settings are chosen to work with the Orange Pi AI Pro debug UART:

```text
Port: first detected COM port, with explicit user selection
Baud: 115200
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

No assumption is made that the device is always `COM3`.

## COM-Port Enumeration

Enumeration should use Windows APIs rather than brute-forcing only a small fixed COM range. The displayed list must include ports such as `COM3`, `COM10`, and higher-numbered ports.

Refreshing the list must preserve the current selection when that port still exists.

## Receive Path

The reader thread reads raw bytes from the open serial handle and forwards byte blocks to the GUI.

Text mode:

- Decode received bytes as UTF-8 when valid.
- Preserve readable ASCII serial logs naturally.
- Replace undecodable byte sequences without crashing the UI.

HEX mode:

- Render bytes as uppercase two-digit hex separated by spaces.

Timestamp mode:

- Prefix newly appended receive blocks/lines with the current local time.

The receive view must be append-only during normal operation and support explicit clearing.

## Send Path

Text mode converts the wxWidgets input string to UTF-8 bytes, then appends the selected line ending:

- None
- CR (`\r`)
- LF (`\n`)
- CRLF (`\r\n`)

HEX mode accepts whitespace-separated or contiguous hexadecimal input. Invalid hex input is rejected with a user-visible error instead of sending partial data.

## Logging

The Save Log action writes the currently displayed receive-console contents to a user-selected UTF-8 text file.

No background logging or rotating-log subsystem is included in the first version.

## Error Handling

Opening errors, disconnected devices, read failures, and write failures are surfaced in the GUI status area and, when appropriate, with a message dialog.

If the device disappears while the port is open:

1. The reader loop terminates.
2. The GUI is notified.
3. The port is closed.
4. Controls return to the disconnected state.

Closing the application must stop the reader thread before destroying the frame.

## Testing

Pure conversion and formatting logic will be kept separately testable from the GUI and OS handle code. Tests cover at minimum:

- Valid HEX parsing.
- Invalid HEX rejection.
- Text line-ending conversion.
- HEX receive formatting.

Hardware-dependent COM-port open/read/write behavior is verified manually on Windows using the Orange Pi AI Pro serial device.

## Documentation

`README.md` will document:

- Required tools: Windows 10/11, Git, CMake, Visual Studio with Desktop development with C++.
- `setup_wxwidgets.bat` usage.
- `build.bat` usage.
- Executable location.
- How to connect to the Orange Pi AI Pro using `COM3` (or the detected port), `115200`, `8N1`, flow control `None`.
- Troubleshooting for missing COM ports and missing wxWidgets checkout.

## Non-Goals for the First Version

The first version will not include:

- Linux or macOS serial backends.
- SSH or TCP terminal support.
- Serial plotting/graphing.
- Scripting/macros.
- Firmware flashing.
- Automatic reconnection loops.
- Bundling wxWidgets source inside Git history.

These can be added later without changing the basic `MainFrame` / `SerialPort` separation.
