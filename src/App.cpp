#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(SerialTestApp);

bool SerialTestApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    auto* frame = new MainFrame();
    frame->Show(true);
    return true;
}
