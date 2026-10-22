// DriverDashboard.cpp  -  TaraMapuan | Driver Workspace: Dashboard
// wxWidgets 3.2+.  Needs UiWidgets.h (shared widgets, palette, icons, shell).
//
// Add to MainFrame.h:   wxWindow* BuildDriverDashboard();
// Standalone: not connected to BuildPage(), ShowPage() or any other page.

#include "MainFrame.h"
#include "UiWidgets.h"

#include <string>
#include <vector>

using namespace ui;

namespace {

// One-line switch: the mockup shows a light-blue trip card; use *wxWHITE for a white one.
const wxColour kTripCardFill = kLpStrip;

struct StopInfo { wxString name, status; };

struct PassengerRow {
    wxString seat, name, studentId;
    bool boarded;
    wxString verifiedAt;
};

// ---- sample data (replace with reservation / trip records later) -----------
struct DriverView {
    wxString driverName  = "Juan Reyes";
    wxString driverCode  = "DR-014";
    wxString initials    = "JR";
    wxString dateText    = "Thu, 08 Oct 2026";
    wxString shuttleId   = "SH-001";
    wxString routeName   = "Campus Loop";
    wxString tripCode    = "R01-0730";
    wxString departure   = "7:30 AM";
    wxString bay         = "Campus Bay A";
    wxString liveUpdated = "7:24 AM";
    wxString tripState   = "Boarding now";
    std::vector<StopInfo> stops = {
        {"Campus",         "At pickup " + MidDot() + " 07:30"},
        {"Library",        "Upcoming " + MidDot() + " 07:34"},
        {"Student Center", "Upcoming " + MidDot() + " 07:38"},
        {"Main Gate",      "Destination " + MidDot() + " 07:42"},
    };

    int seatCapacity = 20;
    int reserved     = 19;
    int boarded      = 16;
    int awaiting     = 3;

    // seat states for the chip strip: index = seat-1  (0 unreserved, 1 boarded, 2 awaiting)
    std::vector<int> seatStates = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,2, 0};

    std::vector<PassengerRow> rows = {
        {"19", "Alex Santos",   "2024-001234", false, ""},
        {"17", "Sam Mendoza",   "2023-008417", false, ""},
        {"18", "Bea Navarro",   "2024-006218", false, ""},
        {"01", "Mia Dela Cruz", "2024-003101", true,  "7:22 AM"},
        {"02", "Leo Fernandez", "2023-001902", true,  "7:22 AM"},
        {"03", "Nina Torres",   "2024-005623", true,  "7:22 AM"},
        {"04", "Ethan Lopez",   "2022-008144", true,  "7:22 AM"},
        {"05", "Chloe Rivera",  "2024-004805", true,  "7:22 AM"},
    };
    int selectedRow = 0;                 // Alex Santos is the passenger being verified
    wxString searchId     = "2024-001234";
    wxString verifyName   = "Alex Santos";
    wxString verifySeat   = "19";
    wxString verifyBooking= "TM-261008-0019";
};

RoundedPanel* SeatChip(wxWindow* parent, const wxString& text, const wxColour& fill, const wxColour& fg) {
    auto* chip = new RoundedPanel(parent, fill, wxColour(), 4);
    chip->SetMinSize(chip->FromDIP(wxSize(22, 22)));
    auto* s = new wxBoxSizer(wxVERTICAL);
    s->AddStretchSpacer(1);
    s->Add(Label(chip, text, 7, fg, true), 0, wxALIGN_CENTER_HORIZONTAL);
    s->AddStretchSpacer(1);
    chip->SetSizer(s);
    return chip;
}

}  // namespace

wxWindow* MainFrame::BuildDriverDashboard() {
    const DriverView v;

    auto* shell = new wxPanel(m_pageHost, wxID_ANY);
    shell->SetBackgroundColour(kLpCanvas);
    auto* shellSizer = new wxBoxSizer(wxHORIZONTAL);

    // =========================================================== SIDEBAR
    // Visual only: nothing here is connected to other pages yet.
    // TODO: wire each handler (e.g. ShowPage(Page::AssignedTrips)) when you are ready.
    const std::vector<NavEntry> nav = {
        {"Dashboard",          kSvgDashboard, [] {}, true},
        {"Assigned Trips",     kSvgCalendar,  [] {}, false},
        {"Current Route",      kSvgRoute,     [] {}, false},
        {"Passenger Manifest", kSvgUsers,     [] {}, false},
        {"Verification",       kSvgScan,      [] {}, false},
        {"Boarding",           kSvgBus,       [] {}, false},
        {"History",            kSvgHistory,   [] {}, false},
    };
    auto logout = [] { /* TODO: logout */ };
    shellSizer->Add(MakeSidebar(shell, "DRIVER WORKSPACE", nav, logout), 0, wxEXPAND);

    // ======================================================== RIGHT COLUMN
    auto* right = new wxPanel(shell, wxID_ANY);
    right->SetBackgroundColour(kLpCanvas);
    auto* rs = new wxBoxSizer(wxVERTICAL);

    rs->Add(MakeTopHeader(right, "Driver portal", kLpGreyBadge, kLpMuted, v.dateText, v.initials,
                          v.driverName, "Driver " + MidDot() + " " + v.driverCode),
            0, wxEXPAND);

    auto* content = new wxScrolledWindow(right, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxVSCROLL | wxBORDER_NONE);
    content->SetScrollRate(0, 10);
    content->SetBackgroundColour(kLpCanvas);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    auto* pad = new wxBoxSizer(wxVERTICAL);
    outer->Add(pad, 1, wxEXPAND | wxALL, 26);

    // ------------------------------------------------------------ title row
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* titles = new wxBoxSizer(wxVERTICAL);
        titles->Add(Label(content, "YOUR ASSIGNED TRIP / TODAY", 7, kLpRed, true));
        titles->Add(Label(content, "Ready for your morning trip.", 22, kLpInk, true), 0, wxTOP, 6);
        titles->Add(Label(content,
                          v.driverName + " " + MidDot() + " Only your current shuttle, route, and passengers are shown here.",
                          9, kLpMuted),
                    0, wxTOP, 6);
        row->Add(titles, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(Badge(content, "On duty " + MidDot() + " " + v.shuttleId, kLpGreenBg, kLpGreen, 7), 0,
                 wxALIGN_TOP | wxTOP, 4);
        pad->Add(row, 0, wxEXPAND);
    }

    // ------------------------------------------------------ active trip card
    {
        pad->AddSpacer(20);
        auto* trip = new RoundedPanel(content, kTripCardFill, wxColour(), 10);
        trip->SetLeftAccent(kLpAccent, 4);
        auto* row = new wxBoxSizer(wxHORIZONTAL);

        auto* left = new wxBoxSizer(wxVERTICAL);
        left->SetMinSize(wxSize(trip->FromDIP(210), 0));
        left->Add(Label(trip, "ACTIVE TRIP " + MidDot() + " " + v.tripCode, 6, kLpNavy, true));
        left->Add(Label(trip, v.routeName, 15, kLpInk, true), 0, wxTOP, 6);
        left->Add(Label(trip, v.shuttleId + " " + MidDot() + " Driver " + v.driverName, 7, kLpMuted), 0, wxTOP, 4);
        left->Add(Badge(trip, v.tripState, wxColour(212, 226, 248), kLpNavy, 7), 0, wxTOP, 12);
        row->Add(left, 0, wxALIGN_TOP);

        auto* rightCol = new wxBoxSizer(wxVERTICAL);
        auto* head = new wxBoxSizer(wxHORIZONTAL);
        head->Add(Label(trip, "Departure " + v.departure + " " + MidDot() + " " + v.bay, 9, kLpInk, true), 0,
                  wxALIGN_CENTER_VERTICAL);
        head->AddStretchSpacer(1);
        head->Add(Label(trip, "Live status " + MidDot() + " Updated " + v.liveUpdated, 7, kLpMuted), 0,
                  wxALIGN_CENTER_VERTICAL);
        rightCol->Add(head, 0, wxEXPAND);

        // route stepper: 4 equal columns, each = circle + name + connector line
        auto* grid = new wxFlexGridSizer(static_cast<int>(v.stops.size()), 6, 0);
        for (size_t i = 0; i < v.stops.size(); ++i) grid->AddGrowableCol(i, 1);
        for (size_t i = 0; i < v.stops.size(); ++i) {
            const bool first = (i == 0);
            const bool last  = (i + 1 == v.stops.size());
            auto* cell = new wxBoxSizer(wxVERTICAL);
            auto* top = new wxBoxSizer(wxHORIZONTAL);

            auto* circle = new RoundedPanel(trip, first ? kLpNavy : *wxWHITE,
                                            first ? wxColour() : wxColour(190, 203, 226), 10);
            circle->SetMinSize(circle->FromDIP(wxSize(20, 20)));
            auto* cs = new wxBoxSizer(wxVERTICAL);
            cs->AddStretchSpacer(1);
            cs->Add(Label(circle, wxString::Format("%zu", i + 1), 7, first ? *wxWHITE : kLpInk, true), 0,
                    wxALIGN_CENTER_HORIZONTAL);
            cs->AddStretchSpacer(1);
            circle->SetSizer(cs);

            top->Add(circle, 0, wxALIGN_CENTER_VERTICAL);
            top->Add(Label(trip, v.stops[i].name, 8, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
            if (!last) {
                auto* line = new wxPanel(trip, wxID_ANY, wxDefaultPosition, wxSize(10, 1));
                line->SetBackgroundColour(wxColour(184, 199, 224));
                top->Add(line, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);
            }
            cell->Add(top, 0, wxEXPAND);
            cell->Add(Label(trip, v.stops[i].status, 6, kLpMuted), 0,
                      wxLEFT | wxTOP, trip->FromDIP(28));
            grid->Add(cell, 1, wxEXPAND);
        }
        rightCol->Add(grid, 0, wxEXPAND | wxTOP, 18);
        row->Add(rightCol, 1, wxEXPAND | wxLEFT, 26);

        auto* tv = new wxBoxSizer(wxVERTICAL);
        tv->AddSpacer(18);
        tv->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 22);
        tv->AddSpacer(18);
        trip->SetSizer(tv);
        pad->Add(trip, 0, wxEXPAND);
    }

    // -------------------------------------------------------- metric cards
    {
        pad->AddSpacer(18);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        const struct { wxString value, text; wxColour colour; } metrics[] = {
            {wxString::Format("%d/%d", v.reserved, v.seatCapacity),
             wxString::Format("Seats reserved %s %d available", MidDot(), v.seatCapacity - v.reserved), kLpNavy},
            {wxString::Format("%d", v.boarded),  "Passengers boarded", kLpGreen},
            {wxString::Format("%d", v.awaiting), "Awaiting check-in",  kLpAmber},
        };
        bool first = true;
        for (const auto& m : metrics) {
            auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
            auto* line = new wxBoxSizer(wxHORIZONTAL);
            line->Add(Label(card, m.value, 20, m.colour, true), 0, wxALIGN_CENTER_VERTICAL);
            line->Add(Label(card, m.text, 8, kLpMuted), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
            auto* cv = new wxBoxSizer(wxVERTICAL);
            cv->AddSpacer(14);
            cv->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 18);
            cv->AddSpacer(14);
            card->SetSizer(cv);
            row->Add(card, 1, wxEXPAND | (first ? 0 : wxLEFT), 18);
            first = false;
        }
        pad->Add(row, 0, wxEXPAND);
    }

    // ------------------------------------------------ lower two-column area
    pad->AddSpacer(18);
    auto* lower = new wxBoxSizer(wxHORIZONTAL);

    // ===== left: passenger manifest & boarding
    {
        auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
        auto* inner = new wxBoxSizer(wxVERTICAL);

        auto* head = new wxBoxSizer(wxHORIZONTAL);
        head->Add(Label(card, "Passenger manifest & boarding", 13, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
        head->AddStretchSpacer(1);
        head->Add(Badge(card, v.shuttleId + " only", kLpGreyBadge, kLpMuted), 0, wxALIGN_CENTER_VERTICAL);
        inner->Add(head, 0, wxEXPAND);

        // seat chip strip (green = boarded, amber = awaiting, gray = unreserved)
        auto* chips = new wxBoxSizer(wxHORIZONTAL);
        chips->Add(Label(card, "SEATS", 6, kLpMuted, true), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        for (size_t i = 0; i < v.seatStates.size(); ++i) {
            wxColour fill = kLpSeatTaken, fg = kLpSeatTakenText;
            if (v.seatStates[i] == 1) { fill = kLpGreenBg; fg = kLpGreen; }
            if (v.seatStates[i] == 2) { fill = kLpAmberBg; fg = kLpAmber; }
            chips->Add(SeatChip(card, wxString::Format("%zu", i + 1), fill, fg), 0, wxRIGHT, 4);
        }
        inner->Add(chips, 0, wxTOP, 14);

        // table: one flex grid, every cell is a tinted panel so rows get backgrounds
        auto* table = new wxFlexGridSizer(5, 0, 0);
        table->AddGrowableCol(0, 1);
        table->AddGrowableCol(1, 4);
        table->AddGrowableCol(2, 3);
        table->AddGrowableCol(3, 4);
        table->AddGrowableCol(4, 3);

        auto cell = [&](const wxColour& bg, std::function<wxWindow*(wxWindow*)> make) {
            auto* p = new wxPanel(card, wxID_ANY);
            p->SetBackgroundColour(bg);
            p->SetMinSize(wxSize(-1, p->FromDIP(34)));
            auto* s = new wxBoxSizer(wxVERTICAL);
            s->AddStretchSpacer(1);
            s->Add(make(p), 0, wxLEFT | wxRIGHT, 12);
            s->AddStretchSpacer(1);
            p->SetSizer(s);
            table->Add(p, 0, wxEXPAND);
        };
        auto text = [](const wxString& t, int pt, const wxColour& c, bool bold) {
            return [=](wxWindow* p) -> wxWindow* { return Label(p, t, pt, c, bold); };
        };

        const wxColour headBg(246, 248, 252);
        for (const char* h : {"SEAT", "STUDENT NAME", "STUDENT ID", "CHECK-IN STATUS", "ACTION"})
            cell(headBg, text(h, 6, kLpMuted, true));

        for (size_t i = 0; i < v.rows.size(); ++i) {
            const auto& r = v.rows[i];
            const wxColour bg = (static_cast<int>(i) == v.selectedRow) ? wxColour(232, 239, 250) : *wxWHITE;
            const bool sel = (static_cast<int>(i) == v.selectedRow);

            cell(bg, text(r.seat, 8, kLpInk, true));
            cell(bg, text(r.name, 8, kLpInk, sel));
            cell(bg, text(r.studentId, 8, kLpMuted, false));
            cell(bg, [&](wxWindow* p) -> wxWindow* {
                return r.boarded ? Badge(p, "Boarded", kLpGreenBg, kLpGreen, 7, kSvgCheck)
                                 : Badge(p, "Awaiting check-in", kLpAmberBg, kLpAmber, 7);
            });
            cell(bg, [&](wxWindow* p) -> wxWindow* {
                if (r.boarded) return Label(p, "Verified " + MidDot() + " " + r.verifiedAt, 7, kLpMuted);
                return Link(p, "Mark No-Show", 7, kLpRed, true, [] { /* TODO: mark no-show */ });
            });
            for (int c = 0; c < 5; ++c) table->Add(Rule(card), 0, wxEXPAND);   // row separator
        }
        inner->Add(table, 0, wxEXPAND | wxTOP, 14);

        // pagination + legend
        auto* foot = new wxBoxSizer(wxHORIZONTAL);
        foot->Add(Label(card, wxString::Format("Showing %zu of %d %s Awaiting check-in first",
                                                v.rows.size(), v.reserved, MidDot()),
                        7, kLpMuted), 0, wxALIGN_CENTER_VERTICAL);
        foot->AddStretchSpacer(1);
        for (int page = 1; page <= 3; ++page) {
            const bool active = (page == 1);
            auto* chip = new RoundedPanel(card, active ? wxColour(212, 226, 248) : kLpSeatTaken, wxColour(), 5);
            chip->SetMinSize(chip->FromDIP(wxSize(26, 26)));
            auto* cs = new wxBoxSizer(wxVERTICAL);
            cs->AddStretchSpacer(1);
            cs->Add(Label(chip, wxString::Format("%d", page), 7, active ? kLpNavy : kLpMuted, true), 0,
                    wxALIGN_CENTER_HORIZONTAL);
            cs->AddStretchSpacer(1);
            chip->SetSizer(cs);
            MakeClickable(chip, [] { /* TODO: change manifest page */ });
            foot->Add(chip, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
        }
        inner->Add(foot, 0, wxEXPAND | wxTOP, 14);
        inner->Add(Label(card,
                         "Seat key: Green = boarded " + MidDot() + " Amber = awaiting check-in " + MidDot() +
                         " Gray = unreserved",
                         6, kLpMuted),
                   0, wxTOP, 12);

        auto* cv = new wxBoxSizer(wxVERTICAL);
        cv->Add(inner, 1, wxEXPAND | wxALL, 20);
        card->SetSizer(cv);
        lower->Add(card, 2, wxEXPAND);
    }
    lower->AddSpacer(18);

    // ===== right: verify passenger + departure checklist
    {
        auto* col = new wxBoxSizer(wxVERTICAL);

        // --- verify passenger
        {
            auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
            auto* inner = new wxBoxSizer(wxVERTICAL);

            auto* head = new wxBoxSizer(wxHORIZONTAL);
            head->Add(Icon(card, kSvgScan, kLpNavy, 20), 0, wxALIGN_CENTER_VERTICAL);
            head->Add(Label(card, "Verify passenger", 13, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
            inner->Add(head, 0, wxEXPAND);
            inner->Add(Label(card, "Scan a digital pass or search a student ID for this trip.", 7, kLpMuted),
                       0, wxTOP, 10);

            // search bar
            auto* search = new RoundedPanel(card, kLpCanvas, kLpBorder, 8);
            auto* sr = new wxBoxSizer(wxHORIZONTAL);
            sr->Add(Icon(search, kSvgSearch, kLpMuted, 16), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
            auto* input = new wxTextCtrl(search, wxID_ANY, v.searchId, wxDefaultPosition, wxDefaultSize,
                                         wxBORDER_NONE);
            input->SetBackgroundColour(kLpCanvas);
            input->SetForegroundColour(kLpInk);
            input->SetFont(wxFont(wxFontInfo(9)));
            sr->Add(input, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
            sr->Add(Link(search, "Find", 8, kLpNavy, true, [] { /* TODO: look up student ID */ }), 0,
                    wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 12);
            auto* sv = new wxBoxSizer(wxVERTICAL);
            sv->AddSpacer(10);
            sv->Add(sr, 0, wxEXPAND);
            sv->AddSpacer(10);
            search->SetSizer(sv);
            inner->Add(search, 0, wxEXPAND | wxTOP, 12);

            inner->Add(new PillButton(card, "Scan QR pass", *wxWHITE, kLpNavy, kLpBorder, 8,
                                      [] { /* TODO: open QR scanner */ },
                                      SvgBundle(kSvgQr, kLpNavy, 14), 10),
                       0, wxEXPAND | wxTOP, 10);

            // valid-pass box
            auto* ok = new RoundedPanel(card, kLpGreenBg, kLpGreenLine, 8);
            auto* os = new wxBoxSizer(wxVERTICAL);
            auto* okHead = new wxBoxSizer(wxHORIZONTAL);
            okHead->Add(Icon(ok, kSvgCheck, kLpGreen, 12), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
            okHead->Add(Label(ok, "Valid pass " + MidDot() + " " + v.verifyName, 8, kLpGreen, true), 0,
                        wxALIGN_CENTER_VERTICAL);
            os->Add(okHead, 0, wxLEFT | wxRIGHT | wxTOP, 14);
            os->Add(Label(ok, v.shuttleId + " " + MidDot() + " Seat " + v.verifySeat + " " + MidDot() +
                              " Payment: Paid", 7, kLpGreen), 0, wxLEFT | wxRIGHT | wxTOP, 14);
            os->Add(Label(ok, v.verifyBooking + " " + MidDot() + " Not yet boarded", 7, kLpGreen), 0,
                    wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 14);
            os->GetItem(size_t(1))->SetBorder(6);
            os->GetItem(size_t(2))->SetBorder(4);
            ok->SetSizer(os);
            inner->Add(ok, 0, wxEXPAND | wxTOP, 14);

            inner->Add(new PillButton(card, "Confirm boarding " + MidDot() + " Seat " + v.verifySeat,
                                      kLpNavy, *wxWHITE, wxColour(), 9,
                                      [] { /* TODO: mark seat as boarded */ },
                                      SvgBundle(kSvgCheck, *wxWHITE, 14), 11),
                       0, wxEXPAND | wxTOP, 14);

            auto* cv = new wxBoxSizer(wxVERTICAL);
            cv->Add(inner, 1, wxEXPAND | wxALL, 20);
            card->SetSizer(cv);
            col->Add(card, 0, wxEXPAND);
        }
        col->AddSpacer(18);

        // --- departure checklist
        {
            auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
            auto* inner = new wxBoxSizer(wxVERTICAL);

            auto* head = new wxBoxSizer(wxHORIZONTAL);
            head->Add(Label(card, "Departure checklist", 10, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
            head->AddStretchSpacer(1);
            head->Add(Badge(card, "Pending", kLpAmberBg, kLpAmber), 0, wxALIGN_CENTER_VERTICAL);
            inner->Add(head, 0, wxEXPAND);

            auto* note = Label(card,
                               wxString::Format("%d passengers still awaiting check-in. Board or mark no-show before departure.",
                                                v.awaiting),
                               7, kLpMuted);
            note->Wrap(card->FromDIP(290));
            inner->Add(note, 0, wxTOP, 12);

            inner->Add(new PillButton(card, "Confirm Departure", kLpRed, *wxWHITE, wxColour(), 9,
                                      [] { /* TODO: confirm departure */ },
                                      SvgBundle(kSvgArrowRight, *wxWHITE, 14), 11),
                       0, wxEXPAND | wxTOP, 14);

            auto* cv = new wxBoxSizer(wxVERTICAL);
            cv->Add(inner, 1, wxEXPAND | wxALL, 20);
            card->SetSizer(cv);
            col->Add(card, 0, wxEXPAND);
        }
        lower->Add(col, 1, wxEXPAND);
    }
    pad->Add(lower, 0, wxEXPAND);

    content->SetSizer(outer);
    content->FitInside();
    rs->Add(content, 1, wxEXPAND);
    right->SetSizer(rs);
    shellSizer->Add(right, 1, wxEXPAND);
    shell->SetSizer(shellSizer);
    return shell;
}