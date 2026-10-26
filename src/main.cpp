#include "MainFrame.h"
#include <wx/image.h>

class TaraShuttleApp final : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        wxInitAllImageHandlers();
        auto* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TaraShuttleApp);
