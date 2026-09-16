# wxWidgets Serial Terminal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Windows serial debugging terminal in C++ with wxWidgets, local wxWidgets bootstrap, Win32 COM-port support, text/HEX send and receive, timestamps, and log saving.

**Architecture:** `MainFrame` owns GUI state and delegates all serial I/O to `SerialPort`. `SerialPort` wraps the Win32 serial API and a background reader thread; pure text/HEX conversion helpers remain independent for unit testing. wxWidgets is downloaded locally into `wxWidgets/` and is never committed.

**Tech Stack:** C++17, wxWidgets 3.2.x, CMake, MSVC/Visual Studio 2022, Win32 serial API, CTest.

**Spec:** `docs/superpowers/specs/2026-09-16-wx-serial-terminal-design.md`

## Global Constraints

- Windows 10/11 only for v1.
- C++17.
- wxWidgets source lives locally at `wxWidgets/` and is ignored by Git.
- `setup_wxwidgets.bat` downloads a fixed stable wxWidgets 3.2.x tag with submodules.
- Build uses local wxWidgets via CMake; no machine-wide wxWidgets install is required.
- Serial defaults: 115200 baud, 8 data bits, no parity, 1 stop bit, flow control None.
- GUI thread must never perform blocking serial reads.
- Reader thread must never touch wxWidgets controls directly.

---

### Task 1: Bootstrap, pure helpers, and tests

**Files:**
- Create: `.gitignore`
- Create: `setup_wxwidgets.bat`
- Create: `build.bat`
- Create: `CMakeLists.txt`
- Create: `src/SerialHelpers.h`
- Create: `src/SerialHelpers.cpp`
- Create: `tests/SerialHelpersTests.cpp`

**Interfaces:**
- Produces: `serial::ParseHex(std::string_view, std::vector<uint8_t>&, std::string&) -> bool`
- Produces: `serial::FormatHex(const std::vector<uint8_t>&) -> std::string`
- Produces: `serial::AppendLineEnding(std::vector<uint8_t>&, serial::LineEnding) -> void`

- [ ] **Step 1: Write failing helper tests**

Create tests covering spaced HEX, contiguous HEX, odd-length rejection, non-HEX rejection, uppercase receive formatting, and None/CR/LF/CRLF line endings.

- [ ] **Step 2: Add minimal CMake test target and run it**

Run on a Windows/MSVC environment:

```bat
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug --target SerialHelpersTests
ctest --test-dir build -C Debug --output-on-failure
```

Expected before helper implementation: compile/test failure.

- [ ] **Step 3: Implement the helper functions**

Keep them independent of wxWidgets and Win32 so the tests remain fast and deterministic.

- [ ] **Step 4: Re-run tests**

Expected: all helper tests pass.

- [ ] **Step 5: Add bootstrap/build scripts**

`setup_wxwidgets.bat` clones wxWidgets tag `v3.2.8.1` recursively into `wxWidgets/` if absent. `build.bat` validates dependencies, configures x64 Release, builds, and prints the executable location.

- [ ] **Step 6: Commit**

```bash
git add .gitignore setup_wxwidgets.bat build.bat CMakeLists.txt src/SerialHelpers.* tests/SerialHelpersTests.cpp
git commit -m "build: add wxWidgets bootstrap and serial helpers"
```

### Task 2: Win32 serial backend

**Files:**
- Create: `src/SerialPort.h`
- Create: `src/SerialPort.cpp`

**Interfaces:**
- Produces: `SerialPort::EnumeratePorts() -> std::vector<std::wstring>`
- Produces: `SerialPort::Open(const SerialSettings&, ReceiveCallback, ErrorCallback) -> bool`
- Produces: `SerialPort::Close() -> void`
- Produces: `SerialPort::Write(const std::vector<uint8_t>&, std::string&) -> bool`
- Produces: `SerialPort::IsOpen() const -> bool`

- [ ] **Step 1: Define settings and public API**

Include baud rate, data bits, parity, stop bits, and flow control enums/settings. Keep Windows HANDLE/thread details private.

- [ ] **Step 2: Implement COM enumeration**

Use `QueryDosDeviceW` to discover `COM*` symbolic links and sort numerically so `COM2` precedes `COM10`.

- [ ] **Step 3: Implement open/configure**

Use `CreateFileW(L"\\\\.\\COMx", ...)`, `GetCommState`, `SetCommState`, `SetCommTimeouts`, and `PurgeComm`.

- [ ] **Step 4: Implement reader thread and clean shutdown**

Read raw byte blocks on a dedicated thread, deliver data only through callbacks, and stop/join before closing the handle.

- [ ] **Step 5: Implement write/error formatting**

Use `WriteFile`; convert `GetLastError()` messages via `FormatMessageW` into readable UTF-8 text.

- [ ] **Step 6: Manual backend verification**

On Windows with Orange Pi attached, confirm `COM3` (or actual device port) appears and opens with 115200/8N1/None.

- [ ] **Step 7: Commit**

```bash
git add src/SerialPort.*
git commit -m "feat: add Win32 serial backend"
```

### Task 3: wxWidgets GUI terminal

**Files:**
- Create: `src/App.h`
- Create: `src/App.cpp`
- Create: `src/MainFrame.h`
- Create: `src/MainFrame.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `SerialPort` and `SerialHelpers`.
- Produces: Windows GUI target `SerialTest`.

- [ ] **Step 1: Create wxWidgets app entry point**

Instantiate `MainFrame` from `wxApp::OnInit()`.

- [ ] **Step 2: Build connection/settings controls**

Add port selector + refresh, baud, data bits, parity, stop bits, flow control, Open/Close, and status label. Defaults must be 115200/8/N/1/None.

- [ ] **Step 3: Build receive panel**

Add large read-only multiline console, Text/HEX display choice, timestamp checkbox, Clear button, and Save Log button.

- [ ] **Step 4: Build send panel**

Add multiline/text input, Text/HEX send choice, line-ending selector, and Send button. Reject invalid HEX with a dialog.

- [ ] **Step 5: Wire serial events safely**

Callbacks from `SerialPort` must marshal to the GUI thread with `CallAfter`. Disconnect errors return the UI to closed state.

- [ ] **Step 6: Handle shutdown**

Close the serial port and join the reader thread before frame destruction.

- [ ] **Step 7: Build verification**

Run:

```bat
build.bat
```

Expected: `SerialTest.exe` is produced in the Release build output.

- [ ] **Step 8: Commit**

```bash
git add src/App.* src/MainFrame.* CMakeLists.txt
git commit -m "feat: add wxWidgets serial terminal UI"
```

### Task 4: Documentation and CI verification

**Files:**
- Create: `README.md`
- Create: `.github/workflows/windows-build.yml`

**Interfaces:**
- CI verifies helper tests and full Windows build using the same bootstrap path users run locally.

- [ ] **Step 1: Document prerequisites and setup**

Document Windows 10/11, Git, CMake, Visual Studio 2022 Desktop development with C++, `setup_wxwidgets.bat`, and `build.bat`.

- [ ] **Step 2: Document Orange Pi quick start**

Explain selecting detected `COM3`/actual port with 115200, 8 data bits, None parity, 1 stop bit, flow control None.

- [ ] **Step 3: Add Windows GitHub Actions build**

Workflow checks out recursively as needed, runs the wxWidgets bootstrap, configures/builds tests, runs CTest, then builds Release `SerialTest`.

- [ ] **Step 4: Verify workflow YAML and repository contents**

Confirm `wxWidgets/` and `build/` are ignored and no downloaded dependency is committed.

- [ ] **Step 5: Commit**

```bash
git add README.md .github/workflows/windows-build.yml
git commit -m "docs: add usage guide and Windows CI"
```
