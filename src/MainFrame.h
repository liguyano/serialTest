#pragma once

#include "SerialHelpers.h"
#include "SerialPort.h"

#include <wx/frame.h>

#include <cstdint>
#include <string>
#include <vector>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxCloseEvent;
class wxCommandEvent;
class wxStaticText;
class wxTextCtrl;

class MainFrame final : public wxFrame {
public:
    MainFrame();
    ~MainFrame() override;

private:
    void BuildUi();
    void RefreshPorts();
    void SetConnectedState(bool connected);

    void OnRefreshPorts(wxCommandEvent& event);
    void OnOpenClose(wxCommandEvent& event);
    void OnSend(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnSaveLog(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void HandleReceivedData(const std::vector<std::uint8_t>& bytes);
    void HandleSerialError(const std::string& error);

    serial::SerialSettings CurrentSettings() const;
    serial::LineEnding CurrentLineEnding() const;

    serial::SerialPort serialPort_;
    bool closing_ = false;

    wxChoice* portChoice_ = nullptr;
    wxChoice* baudChoice_ = nullptr;
    wxChoice* dataBitsChoice_ = nullptr;
    wxChoice* parityChoice_ = nullptr;
    wxChoice* stopBitsChoice_ = nullptr;
    wxChoice* flowControlChoice_ = nullptr;
    wxButton* refreshButton_ = nullptr;
    wxButton* openCloseButton_ = nullptr;
    wxStaticText* statusLabel_ = nullptr;

    wxChoice* receiveModeChoice_ = nullptr;
    wxCheckBox* timestampCheckBox_ = nullptr;
    wxTextCtrl* receiveText_ = nullptr;

    wxChoice* sendModeChoice_ = nullptr;
    wxChoice* lineEndingChoice_ = nullptr;
    wxTextCtrl* sendText_ = nullptr;
    wxButton* sendButton_ = nullptr;
};
