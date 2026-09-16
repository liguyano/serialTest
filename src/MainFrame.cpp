#include "MainFrame.h"

#include "SerialHelpers.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/datetime.h>
#include <wx/filedlg.h>
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

wxChoice* CreateChoice(wxWindow* parent, std::initializer_list<const char*> values) {
    auto* choice = new wxChoice(parent, wxID_ANY);
    for (const char* value : values) {
        choice->Append(wxString::FromUTF8(value));
    }
    return choice;
}

wxString BytesToReadableText(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }

    wxString decoded = wxString::FromUTF8(
        reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!decoded.empty()) {
        return decoded;
    }

    wxString fallback;
    for (std::uint8_t byte : bytes) {
        if (byte == '\r' || byte == '\n' || byte == '\t' ||
            (byte >= 0x20 && byte <= 0x7E)) {
            fallback.Append(wxUniChar(byte));
        } else {
            fallback.Append(wxUniChar(0xFFFD));
        }
    }
    return fallback;
}

}  // namespace

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "SerialTest - wxWidgets Serial Terminal",
              wxDefaultPosition, wxSize(1000, 700)) {
    BuildUi();
    RefreshPorts();
    SetConnectedState(false);
    Centre();

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
}

MainFrame::~MainFrame() {
    closing_ = true;
    serialPort_.Close();
}

void MainFrame::BuildUi() {
    auto* panel = new wxPanel(this);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* connectionBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Connection");
    auto* portRow = new wxBoxSizer(wxHORIZONTAL);
    portRow->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    portChoice_ = new wxChoice(panel, wxID_ANY);
    portRow->Add(portChoice_, 0, wxRIGHT, 6);

    refreshButton_ = new wxButton(panel, wxID_ANY, "Refresh");
    portRow->Add(refreshButton_, 0, wxRIGHT, 10);

    openCloseButton_ = new wxButton(panel, wxID_ANY, "Open");
    portRow->Add(openCloseButton_, 0, wxRIGHT, 12);

    statusLabel_ = new wxStaticText(panel, wxID_ANY, "Disconnected");
    portRow->Add(statusLabel_, 0, wxALIGN_CENTER_VERTICAL);
    connectionBox->Add(portRow, 0, wxEXPAND | wxALL, 6);

    auto* settingsRow = new wxBoxSizer(wxHORIZONTAL);

    settingsRow->Add(new wxStaticText(panel, wxID_ANY, "Baud:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    baudChoice_ = CreateChoice(panel, {"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    baudChoice_->SetStringSelection("115200");
    settingsRow->Add(baudChoice_, 0, wxRIGHT, 10);

    settingsRow->Add(new wxStaticText(panel, wxID_ANY, "Data bits:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    dataBitsChoice_ = CreateChoice(panel, {"7", "8"});
    dataBitsChoice_->SetStringSelection("8");
    settingsRow->Add(dataBitsChoice_, 0, wxRIGHT, 10);

    settingsRow->Add(new wxStaticText(panel, wxID_ANY, "Parity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    parityChoice_ = CreateChoice(panel, {"None", "Odd", "Even"});
    parityChoice_->SetSelection(0);
    settingsRow->Add(parityChoice_, 0, wxRIGHT, 10);

    settingsRow->Add(new wxStaticText(panel, wxID_ANY, "Stop bits:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    stopBitsChoice_ = CreateChoice(panel, {"1", "2"});
    stopBitsChoice_->SetSelection(0);
    settingsRow->Add(stopBitsChoice_, 0, wxRIGHT, 10);

    settingsRow->Add(new wxStaticText(panel, wxID_ANY, "Flow control:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    flowControlChoice_ = CreateChoice(panel, {"None", "RTS/CTS", "XON/XOFF"});
    flowControlChoice_->SetSelection(0);
    settingsRow->Add(flowControlChoice_, 0);

    connectionBox->Add(settingsRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    root->Add(connectionBox, 0, wxEXPAND | wxALL, 8);

    auto* receiveBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Receive");
    auto* receiveToolbar = new wxBoxSizer(wxHORIZONTAL);
    receiveToolbar->Add(new wxStaticText(panel, wxID_ANY, "Display:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    receiveModeChoice_ = CreateChoice(panel, {"Text", "HEX"});
    receiveModeChoice_->SetSelection(0);
    receiveToolbar->Add(receiveModeChoice_, 0, wxRIGHT, 10);

    timestampCheckBox_ = new wxCheckBox(panel, wxID_ANY, "Timestamp");
    receiveToolbar->Add(timestampCheckBox_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    auto* clearButton = new wxButton(panel, wxID_ANY, "Clear");
    receiveToolbar->Add(clearButton, 0, wxRIGHT, 6);
    auto* saveButton = new wxButton(panel, wxID_ANY, "Save log...");
    receiveToolbar->Add(saveButton, 0);
    receiveBox->Add(receiveToolbar, 0, wxEXPAND | wxALL, 6);

    receiveText_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_DONTWRAP);
    receiveBox->Add(receiveText_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    root->Add(receiveBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* sendBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Send");
    auto* sendToolbar = new wxBoxSizer(wxHORIZONTAL);
    sendToolbar->Add(new wxStaticText(panel, wxID_ANY, "Mode:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    sendModeChoice_ = CreateChoice(panel, {"Text", "HEX"});
    sendModeChoice_->SetSelection(0);
    sendToolbar->Add(sendModeChoice_, 0, wxRIGHT, 10);

    sendToolbar->Add(new wxStaticText(panel, wxID_ANY, "Line ending:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    lineEndingChoice_ = CreateChoice(panel, {"None", "CR", "LF", "CRLF"});
    lineEndingChoice_->SetSelection(0);
    sendToolbar->Add(lineEndingChoice_, 0);
    sendBox->Add(sendToolbar, 0, wxEXPAND | wxALL, 6);

    auto* sendRow = new wxBoxSizer(wxHORIZONTAL);
    sendText_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE);
    sendRow->Add(sendText_, 1, wxEXPAND | wxRIGHT, 6);
    sendButton_ = new wxButton(panel, wxID_ANY, "Send");
    sendRow->Add(sendButton_, 0, wxALIGN_CENTER_VERTICAL);
    sendBox->Add(sendRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    root->Add(sendBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(root);

    refreshButton_->Bind(wxEVT_BUTTON, &MainFrame::OnRefreshPorts, this);
    openCloseButton_->Bind(wxEVT_BUTTON, &MainFrame::OnOpenClose, this);
    sendButton_->Bind(wxEVT_BUTTON, &MainFrame::OnSend, this);
    clearButton->Bind(wxEVT_BUTTON, &MainFrame::OnClear, this);
    saveButton->Bind(wxEVT_BUTTON, &MainFrame::OnSaveLog, this);
    sendModeChoice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        lineEndingChoice_->Enable(sendModeChoice_->GetSelection() == 0);
    });
}

void MainFrame::RefreshPorts() {
    const wxString previous = portChoice_->GetStringSelection();
    portChoice_->Clear();

    for (const std::wstring& port : serial::SerialPort::EnumeratePorts()) {
        portChoice_->Append(wxString(port.c_str()));
    }

    const int previousIndex = portChoice_->FindString(previous);
    if (previousIndex != wxNOT_FOUND) {
        portChoice_->SetSelection(previousIndex);
    } else if (portChoice_->GetCount() > 0) {
        portChoice_->SetSelection(0);
    }

    if (!serialPort_.IsOpen()) {
        statusLabel_->SetLabel(portChoice_->GetCount() == 0 ? "No COM ports detected" : "Disconnected");
    }
}

void MainFrame::SetConnectedState(bool connected) {
    portChoice_->Enable(!connected);
    refreshButton_->Enable(!connected);
    baudChoice_->Enable(!connected);
    dataBitsChoice_->Enable(!connected);
    parityChoice_->Enable(!connected);
    stopBitsChoice_->Enable(!connected);
    flowControlChoice_->Enable(!connected);

    openCloseButton_->SetLabel(connected ? "Close" : "Open");
    statusLabel_->SetLabel(connected ? "Connected" : "Disconnected");
    sendButton_->Enable(connected);
}

void MainFrame::OnRefreshPorts(wxCommandEvent&) {
    RefreshPorts();
}

void MainFrame::OnOpenClose(wxCommandEvent&) {
    if (serialPort_.IsOpen()) {
        serialPort_.Close();
        SetConnectedState(false);
        return;
    }

    if (portChoice_->GetSelection() == wxNOT_FOUND) {
        wxMessageBox("No COM port is selected.", "SerialTest", wxOK | wxICON_WARNING, this);
        return;
    }

    const serial::SerialSettings settings = CurrentSettings();
    const bool opened = serialPort_.Open(
        settings,
        [this](std::vector<std::uint8_t> bytes) {
            CallAfter([this, bytes = std::move(bytes)]() {
                HandleReceivedData(bytes);
            });
        },
        [this](std::string error) {
            CallAfter([this, error = std::move(error)]() {
                HandleSerialError(error);
            });
        });

    SetConnectedState(opened);
}

void MainFrame::OnSend(wxCommandEvent&) {
    if (!serialPort_.IsOpen()) {
        wxMessageBox("Open a serial port before sending data.", "SerialTest",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    std::vector<std::uint8_t> bytes;
    const wxString value = sendText_->GetValue();
    const wxScopedCharBuffer utf8 = value.ToUTF8();

    if (sendModeChoice_->GetSelection() == 1) {
        const std::string hexText = utf8.data() == nullptr
                                        ? std::string()
                                        : std::string(utf8.data(), utf8.length());
        std::string parseError;
        if (!serial::ParseHex(hexText, bytes, parseError)) {
            wxMessageBox(wxString::FromUTF8(parseError), "Invalid HEX input",
                         wxOK | wxICON_ERROR, this);
            return;
        }
    } else {
        if (utf8.data() != nullptr) {
            const auto* begin = reinterpret_cast<const std::uint8_t*>(utf8.data());
            bytes.assign(begin, begin + utf8.length());
        }
        serial::AppendLineEnding(bytes, CurrentLineEnding());
    }

    std::string writeError;
    if (!serialPort_.Write(bytes, writeError)) {
        wxMessageBox(wxString::FromUTF8(writeError), "Serial write failed",
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnClear(wxCommandEvent&) {
    receiveText_->Clear();
}

void MainFrame::OnSaveLog(wxCommandEvent&) {
    wxFileDialog dialog(this, "Save serial log", wxEmptyString, "serial-log.txt",
                        "Text files (*.txt)|*.txt|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    wxFFile file(dialog.GetPath(), "wb");
    if (!file.IsOpened()) {
        wxMessageBox("Could not open the selected file for writing.", "Save log",
                     wxOK | wxICON_ERROR, this);
        return;
    }

    const wxScopedCharBuffer utf8 = receiveText_->GetValue().ToUTF8();
    if (utf8.data() != nullptr && utf8.length() > 0) {
        const std::size_t written = file.Write(utf8.data(), utf8.length());
        if (written != utf8.length()) {
            wxMessageBox("The log could not be written completely.", "Save log",
                         wxOK | wxICON_ERROR, this);
        }
    }
}

void MainFrame::OnClose(wxCloseEvent& event) {
    closing_ = true;
    serialPort_.Close();
    event.Skip();
}

void MainFrame::HandleReceivedData(const std::vector<std::uint8_t>& bytes) {
    wxString text;
    if (receiveModeChoice_->GetSelection() == 1) {
        text = wxString::FromUTF8(serial::FormatHex(bytes));
        if (!text.empty()) {
            text += timestampCheckBox_->GetValue() ? "\n" : " ";
        }
    } else {
        text = BytesToReadableText(bytes);
    }

    if (timestampCheckBox_->GetValue() && !text.empty()) {
        text = "[" + wxDateTime::Now().Format("%H:%M:%S") + "] " + text;
    }

    receiveText_->AppendText(text);
    receiveText_->ShowPosition(receiveText_->GetLastPosition());
}

void MainFrame::HandleSerialError(const std::string& error) {
    serialPort_.Close();
    SetConnectedState(false);
    if (!closing_) {
        wxMessageBox(wxString::FromUTF8(error), "Serial error",
                     wxOK | wxICON_ERROR, this);
    }
}

serial::SerialSettings MainFrame::CurrentSettings() const {
    serial::SerialSettings settings;
    const wxString port = portChoice_->GetStringSelection();
    settings.port.assign(port.wc_str());

    unsigned long baud = 115200;
    baudChoice_->GetStringSelection().ToULong(&baud);
    settings.baudRate = static_cast<std::uint32_t>(baud);

    long dataBits = 8;
    dataBitsChoice_->GetStringSelection().ToLong(&dataBits);
    settings.dataBits = static_cast<std::uint8_t>(dataBits);

    switch (parityChoice_->GetSelection()) {
        case 1: settings.parity = serial::Parity::Odd; break;
        case 2: settings.parity = serial::Parity::Even; break;
        default: settings.parity = serial::Parity::None; break;
    }

    settings.stopBits = stopBitsChoice_->GetSelection() == 1
                            ? serial::StopBits::Two
                            : serial::StopBits::One;

    switch (flowControlChoice_->GetSelection()) {
        case 1: settings.flowControl = serial::FlowControl::Hardware; break;
        case 2: settings.flowControl = serial::FlowControl::Software; break;
        default: settings.flowControl = serial::FlowControl::None; break;
    }

    return settings;
}

serial::LineEnding MainFrame::CurrentLineEnding() const {
    switch (lineEndingChoice_->GetSelection()) {
        case 1: return serial::LineEnding::CR;
        case 2: return serial::LineEnding::LF;
        case 3: return serial::LineEnding::CRLF;
        default: return serial::LineEnding::None;
    }
}
