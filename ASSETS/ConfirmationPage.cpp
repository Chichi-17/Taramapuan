// ConfirmationPage.cpp  -  TaraMapuan | Passenger Portal: Reservation Confirmation
// wxWidgets 3.2+
//
// Add to MainFrame.h:   wxWindow* BuildConfirmationPage();
// Not wired into LandingPage.cpp. To show it later, return BuildConfirmationPage()
// from BuildPage() for whichever Page value you use, and hide the frame's own
// sidebar/top bar for that page (this view draws its own shell).
//
// NOTE: the helper widgets below mirror LandingPage.cpp. They live in an anonymous
// namespace so the two files do not clash. Once you are happy, move them into a
// shared UiWidgets.h and delete the copies.

#include "MainFrame.h"

#include <wx/bmpbndl.h>
#include <wx/dcbuffer.h>
#include <wx/statbmp.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace {

// ---- palette (same as landing page + a few additions) ----------------------
const wxColour kLpNavy(15, 42, 82);
const wxColour kLpNavyLight(36, 68, 122);     // active nav item
const wxColour kLpNavyPanel(27, 58, 104);     // "Need a hand?" panel
const wxColour kLpRed(190, 24, 58);
const wxColour kLpInk(24, 38, 64);
const wxColour kLpMuted(106, 118, 140);
const wxColour kLpNavMuted(200, 214, 236);
const wxColour kLpStrip(232, 239, 250);
const wxColour kLpGreen(22, 128, 92);
const wxColour kLpGreenBg(230, 246, 238);
const wxColour kLpGreenLine(198, 231, 213);
const wxColour kLpBorder(222, 229, 240);
const wxColour kLpSoft(247, 249, 252);
const wxColour kLpCanvas(245, 247, 251);
const wxColour kLpSeatTaken(236, 240, 247);
const wxColour kLpSeatTakenText(150, 160, 178);

// ---- glyphs ----------------------------------------------------------------
wxString MidDot()    { return wxString::FromUTF8("\xC2\xB7"); }          // ·
wxString Peso()      { return wxString::FromUTF8("\xE2\x82\xB1"); }      // ₱
wxString ArrowLeft() { return wxString::FromUTF8("\xE2\x86\x90"); }      // ←
wxString ArrowDown() { return wxString::FromUTF8("\xE2\x86\x93"); }      // ↓
wxString Bullets()   { return wxString::FromUTF8("\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2"); }  // ••••
wxString Money(double v) { return Peso() + wxString::Format("%.2f", v); }

// ---- SVG icon bodies (24x24 viewBox, stroke based) -------------------------
const char* kSvgBus        = R"(<rect x='4' y='3' width='16' height='15' rx='3'/><path d='M4 11h16M8 18v2M16 18v2'/><circle cx='8' cy='14.5' r='.6'/><circle cx='16' cy='14.5' r='.6'/>)";
const char* kSvgCheck      = R"(<path d='M5 12.5l4.5 4.5L19 7.5'/>)";
const char* kSvgCheckCircle= R"(<circle cx='12' cy='12' r='9'/><path d='M8 12.5l3 3 5-6'/>)";
const char* kSvgBell       = R"(<path d='M6 16v-5a6 6 0 0 1 12 0v5l1.5 2h-15z'/><path d='M10 21h4'/>)";
const char* kSvgQr         = R"(<rect x='4' y='4' width='6' height='6' rx='1'/><rect x='14' y='4' width='6' height='6' rx='1'/><rect x='4' y='14' width='6' height='6' rx='1'/><path d='M14 14h3v3M20 14v.01M14 20h3M20 17v3'/>)";
const char* kSvgWallet     = R"(<path d='M4 7h14a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2z'/><path d='M4 7l2-3h10M16 13h4v3h-4a1.5 1.5 0 0 1 0-3z'/>)";
const char* kSvgReceipt    = R"(<path d='M6 3h12v18l-3-2-3 2-3-2-3 2z'/><path d='M9 8h6M9 12h6'/>)";
const char* kSvgGradCap    = R"(<path d='M2 9l10-5 10 5-10 5z'/><path d='M6 11.5V16c0 1.5 3 3 6 3s6-1.5 6-3v-4.5M22 9v6'/>)";
const char* kSvgDashboard  = R"(<rect x='4' y='4' width='6.5' height='6.5' rx='1.5'/><rect x='13.5' y='4' width='6.5' height='6.5' rx='1.5'/><rect x='4' y='13.5' width='6.5' height='6.5' rx='1.5'/><rect x='13.5' y='13.5' width='6.5' height='6.5' rx='1.5'/>)";
const char* kSvgRoute      = R"(<circle cx='6' cy='6' r='2.5'/><circle cx='18' cy='18' r='2.5'/><path d='M8.5 6H15a3 3 0 0 1 0 6H9a3 3 0 0 0 0 6h6.5'/>)";
const char* kSvgCalendar   = R"(<rect x='4' y='5' width='16' height='15' rx='2'/><path d='M4 10h16M9 3v4M15 3v4'/>)";
const char* kSvgTicket     = R"(<path d='M3 9V7a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2v2a2.5 2.5 0 0 0 0 5v3a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-3a2.5 2.5 0 0 0 0-5z'/><path d='M9 8v8'/>)";
const char* kSvgUser       = R"(<circle cx='12' cy='8' r='4'/><path d='M4 20c1-4 4-6 8-6s7 2 8 6'/>)";
const char* kSvgHeadset    = R"(<path d='M4 14v-2a8 8 0 0 1 16 0v2'/><rect x='3' y='14' width='4' height='6' rx='1.5'/><rect x='17' y='14' width='4' height='6' rx='1.5'/>)";
const char* kSvgLogout     = R"(<path d='M10 4H6a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h4M15 8l4 4-4 4M19 12H9'/>)";
const char* kSvgChevron    = R"(<path d='M9 6l6 6-6 6'/>)";

wxBitmapBundle SvgBundle(const char* body, const wxColour& stroke, int size) {
    const std::string svg =
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='" +
        stroke.GetAsString(wxC2S_HTML_SYNTAX).ToStdString() +
        "' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>" + body + "</svg>";
    return wxBitmapBundle::FromSVG(svg.c_str(), wxSize(size, size));  // size is in DIP
}

wxStaticBitmap* Icon(wxWindow* parent, const char* body, const wxColour& stroke, int size) {
    return new wxStaticBitmap(parent, wxID_ANY, SvgBundle(body, stroke, size));
}

// ---- text helpers ----------------------------------------------------------
wxStaticText* Label(wxWindow* parent, const wxString& text, int pt, const wxColour& colour,
                    bool bold = false) {
    auto* s = new wxStaticText(parent, wxID_ANY, text);
    s->SetFont(bold ? wxFont(wxFontInfo(pt).Bold()) : wxFont(wxFontInfo(pt)));
    s->SetForegroundColour(colour);
    return s;
}

wxStaticText* Link(wxWindow* parent, const wxString& text, int pt, const wxColour& colour,
                   bool bold, std::function<void()> onClick) {
    auto* s = Label(parent, text, pt, colour, bold);
    s->SetCursor(wxCursor(wxCURSOR_HAND));
    s->Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
    return s;
}

void MakeClickable(wxWindow* w, const std::function<void()>& onClick) {
    w->SetCursor(wxCursor(wxCURSOR_HAND));
    w->Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
    for (wxWindow* child : w->GetChildren()) MakeClickable(child, onClick);
}

wxPanel* Rule(wxWindow* parent) {
    auto* r = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    r->SetMinSize(wxSize(-1, 1));
    r->SetBackgroundColour(kLpBorder);
    return r;
}

// ---- RoundedPanel: children created afterwards inherit its fill colour -----
class RoundedPanel : public wxPanel {
public:
    RoundedPanel(wxWindow* parent, const wxColour& fill, const wxColour& border, int radius)
        : wxPanel(parent, wxID_ANY), m_fill(fill), m_border(border), m_radius(radius) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(fill);
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
            dc.Clear();
            dc.SetBrush(wxBrush(m_fill));
            dc.SetPen(m_border.IsOk() ? wxPen(m_border) : *wxTRANSPARENT_PEN);
            const wxSize s = GetClientSize();
            dc.DrawRoundedRectangle(0, 0, s.x, s.y, m_radius);
        });
    }
private:
    wxColour m_fill, m_border;
    int m_radius;
};

// ---- PillButton (now with optional trailing icon + custom vertical padding) -
class PillButton : public wxPanel {
public:
    PillButton(wxWindow* parent, const wxString& label, const wxColour& fill,
               const wxColour& text, const wxColour& border, int pt,
               std::function<void()> onClick,
               const wxBitmapBundle& icon = wxBitmapBundle(), int vpad = 10)
        : wxPanel(parent, wxID_ANY), m_label(label), m_fill(fill), m_text(text),
          m_border(border), m_icon(icon) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetFont(wxFont(wxFontInfo(pt).Bold()));
        SetCursor(wxCursor(wxCURSOR_HAND));
        wxClientDC measure(this);
        measure.SetFont(GetFont());
        const wxSize t = measure.GetTextExtent(label);
        int w = t.x + FromDIP(32);
        int h = t.y + FromDIP(2 * vpad);
        if (m_icon.IsOk()) {
            const wxSize is = m_icon.GetPreferredBitmapSizeFor(this);
            w += is.x + FromDIP(8);
            h = std::max(h, is.y + FromDIP(2 * vpad));
        }
        SetMinSize(wxSize(w, h));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) { OnPaint(); });
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hover = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hover = false; Refresh(); });
        Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
    }
private:
    void OnPaint() {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        wxColour f = m_fill;
        if (m_hover) f = (f == *wxWHITE) ? wxColour(240, 244, 250) : f.ChangeLightness(112);
        dc.SetBrush(wxBrush(f));
        dc.SetPen(m_border.IsOk() ? wxPen(m_border) : *wxTRANSPARENT_PEN);
        const wxSize s = GetClientSize();
        dc.DrawRoundedRectangle(0, 0, s.x, s.y, 6);
        dc.SetFont(GetFont());
        dc.SetTextForeground(m_text);
        const wxSize t = dc.GetTextExtent(m_label);
        wxSize is(0, 0);
        const int gap = FromDIP(8);
        if (m_icon.IsOk()) is = m_icon.GetPreferredBitmapSizeFor(this);
        const int total = t.x + (m_icon.IsOk() ? gap + is.x : 0);
        const int x0 = (s.x - total) / 2;
        dc.DrawText(m_label, x0, (s.y - t.y) / 2);
        if (m_icon.IsOk())
            dc.DrawBitmap(m_icon.GetBitmapFor(this), x0 + t.x + gap, (s.y - is.y) / 2, true);
    }
    wxString m_label;
    wxColour m_fill, m_text, m_border;
    wxBitmapBundle m_icon;
    bool m_hover = false;
};

// ---- Seat tile ---------------------------------------------------------------
enum class SeatState { Available, Occupied, Mine };

class SeatTile : public wxPanel {
public:
    SeatTile(wxWindow* parent, int number, SeatState state)
        : wxPanel(parent, wxID_ANY), m_label(wxString::Format("%02d", number)), m_state(state) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetFont(wxFont(wxFontInfo(8).Bold()));
        SetMinSize(FromDIP(wxSize(40, 40)));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
            dc.Clear();
            wxColour fill = *wxWHITE, border = kLpNavy, fg = kLpNavy;
            if (m_state == SeatState::Occupied) { fill = kLpSeatTaken; border = kLpBorder; fg = kLpSeatTakenText; }
            if (m_state == SeatState::Mine)     { fill = kLpNavy; border = kLpNavy; fg = *wxWHITE; }
            dc.SetBrush(wxBrush(fill));
            dc.SetPen(wxPen(border));
            const wxSize s = GetClientSize();
            dc.DrawRoundedRectangle(0, 0, s.x, s.y, 8);
            dc.SetFont(GetFont());
            dc.SetTextForeground(fg);
            const wxSize t = dc.GetTextExtent(m_label);
            dc.DrawText(m_label, (s.x - t.x) / 2, (s.y - t.y) / 2);
        });
    }
private:
    wxString m_label;
    SeatState m_state;
};

RoundedPanel* Badge(wxWindow* parent, const wxString& text, const wxColour& fill,
                    const wxColour& fg, int pt = 7) {
    auto* b = new RoundedPanel(parent, fill, wxColour(), 6);
    auto* s = new wxBoxSizer(wxVERTICAL);
    s->AddSpacer(4);
    s->Add(Label(b, text, pt, fg, true), 0, wxLEFT | wxRIGHT, 10);
    s->AddSpacer(4);
    b->SetSizer(s);
    return b;
}

RoundedPanel* Swatch(wxWindow* parent, const wxColour& fill, const wxColour& border) {
    auto* p = new RoundedPanel(parent, fill, border, 3);
    p->SetMinSize(p->FromDIP(wxSize(14, 14)));
    return p;
}

RoundedPanel* Dot(wxWindow* parent, const wxColour& colour, int d) {
    auto* p = new RoundedPanel(parent, colour, wxColour(), d / 2);
    p->SetMinSize(p->FromDIP(wxSize(d, d)));
    return p;
}

// ---- sample data (replace with a real ReservationRecord later) -------------
struct ConfirmationView {
    wxString firstName     = "Alex";
    wxString fullName      = "Alex Santos";
    wxString studentId     = "2024-001234";
    wxString initials      = "AS";
    wxString dateText      = "Thu, 08 Oct 2026";
    wxString bookingId     = "TM-261008-0019";
    int      seatNo        = 19;
    int      seatCount     = 20;
    int      occupiedBefore= 18;
    wxString seatNote      = "Aisle";
    wxString shuttleId     = "SH-001";
    wxString routeName     = "Campus Loop";
    wxString routeCode     = "R01";
    wxString destination   = "Main Gate";
    wxString departDate    = "Today, 08 Oct";
    wxString departTime    = "7:30 AM";
    wxString bay           = "Campus Bay A";
    wxString driver        = "Juan Reyes";
    double   fare          = 20.0;
    double   fee           = 0.0;
    wxString payMethod     = "GCash";
    wxString payLast4      = "4821";
    wxString payRef        = "GC-1080019";
    wxString paidAt        = "7:12 AM";
    wxString arriveBy      = "7:20 AM";
    std::vector<wxString>  stops = {"Campus", "Library", "Student Center", "Main Gate"};
};

}  // namespace

wxWindow* MainFrame::BuildConfirmationPage() {
    const ConfirmationView v;

    auto* shell = new wxPanel(m_pageHost, wxID_ANY);
    shell->SetBackgroundColour(kLpCanvas);
    auto* shellSizer = new wxBoxSizer(wxHORIZONTAL);

    // =========================================================== SIDEBAR
    auto* side = new wxPanel(shell, wxID_ANY);
    side->SetBackgroundColour(kLpNavy);
    side->SetMinSize(wxSize(side->FromDIP(204), -1));
    auto* ss = new wxBoxSizer(wxVERTICAL);

    {   // brand
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* logo = new RoundedPanel(side, kLpRed, wxColour(), 9);
        auto* ls = new wxBoxSizer(wxVERTICAL);
        ls->Add(Icon(logo, kSvgBus, *wxWHITE, 22), 0, wxALL, 9);
        logo->SetSizer(ls);
        row->Add(logo, 0, wxALIGN_CENTER_VERTICAL);
        row->Add(Label(side, "TaraMapuan", 17, *wxWHITE, true), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        ss->Add(row, 0, wxLEFT | wxRIGHT | wxTOP, 18);
    }
    {   // portal caption
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* bar = new wxPanel(side, wxID_ANY, wxDefaultPosition, wxSize(3, 14));
        bar->SetBackgroundColour(kLpRed);
        row->Add(bar, 0, wxALIGN_CENTER_VERTICAL);
        row->Add(Label(side, "PASSENGER PORTAL", 7, kLpNavMuted, true), 0,
                 wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        ss->AddSpacer(22);
        ss->Add(row, 0, wxLEFT | wxRIGHT, 18);
        ss->AddSpacer(14);
        ss->Add(Rule(side), 0, wxEXPAND | wxLEFT | wxRIGHT, 14)->GetWindow()->SetBackgroundColour(kLpNavyLight);
        ss->AddSpacer(14);
    }

    auto navItem = [&](const wxString& text, const char* svg, std::function<void()> go, bool active) {
        const wxColour fill = active ? kLpNavyLight : kLpNavy;
        auto* item = new RoundedPanel(side, fill, wxColour(), 8);
        item->SetMinSize(wxSize(-1, item->FromDIP(42)));
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* bar = new wxPanel(item, wxID_ANY, wxDefaultPosition, wxSize(3, 22));
        bar->SetBackgroundColour(active ? kLpRed : fill);      // red left-border indicator
        row->Add(bar, 0, wxALIGN_CENTER_VERTICAL);
        const wxColour tint = active ? *wxWHITE : kLpNavMuted;
        row->Add(Icon(item, svg, tint, 18), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        row->Add(Label(item, text, 9, tint, active), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        item->SetSizer(row);
        MakeClickable(item, go);
        ss->Add(item, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);
        return item;
    };
    navItem("Dashboard",       kSvgDashboard, [this] { ShowPage(Page::StudentDashboard); }, false);
    navItem("Find Route",      kSvgRoute,     [this] { ShowPage(Page::Reserve); },          false);
    navItem("Schedule",        kSvgCalendar,  [this] { ShowPage(Page::Schedules); },        false);
    navItem("My Reservations", kSvgTicket,    [this] { ShowPage(Page::Reservations); },     true);
    navItem("Digital Pass",    kSvgQr,        [this] { ShowPage(Page::DigitalPass); },      false);
    navItem("Profile",         kSvgUser,      [this] { ShowPage(Page::Settings); },         false);

    ss->AddStretchSpacer(1);

    {   // "Need a hand?"
        auto* help = new RoundedPanel(side, kLpNavyPanel, wxColour(), 12);
        auto* hs = new wxBoxSizer(wxVERTICAL);
        hs->Add(Icon(help, kSvgHeadset, *wxWHITE, 18), 0, wxLEFT | wxRIGHT | wxTOP, 14);
        hs->Add(Label(help, "Need a hand?", 9, *wxWHITE, true), 0, wxLEFT | wxRIGHT | wxTOP, 14);
        hs->Add(Label(help, "Campus transport desk", 7, kLpNavMuted), 0, wxLEFT | wxRIGHT | wxTOP, 14);
        hs->Add(Label(help, wxString::FromUTF8("Mon-Sat \xC2\xB7 6:00 AM-8:00 PM"), 7, kLpNavMuted),
                0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 14);
        help->SetSizer(hs);
        ss->Add(help, 0, wxEXPAND | wxLEFT | wxRIGHT, 14);
    }
    {   // logout
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Icon(side, kSvgLogout, kLpNavMuted, 18), 0, wxALIGN_CENTER_VERTICAL);
        row->Add(Label(side, "Logout", 9, kLpNavMuted), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        ss->Add(row, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 22);
        for (size_t i = 0; i < row->GetItemCount(); ++i)
            if (auto* w = row->GetItem(i)->GetWindow())
                MakeClickable(w, [this] {
                    m_userRole = UserRole::None;
                    m_selectedRole = UserRole::None;
                    m_currentUserId.clear();
                    m_selectedSchedule = -1;
                    m_selectedReservationId.clear();
                    ShowPage(Page::Landing);
                });
    }
    side->SetSizer(ss);
    shellSizer->Add(side, 0, wxEXPAND);

    // ======================================================== RIGHT COLUMN
    auto* right = new wxPanel(shell, wxID_ANY);
    right->SetBackgroundColour(kLpCanvas);
    auto* rs = new wxBoxSizer(wxVERTICAL);

    // ----------------------------------------------------------- top header
    {
        auto* header = new wxPanel(right, wxID_ANY);
        header->SetBackgroundColour(*wxWHITE);
        auto* hs = new wxBoxSizer(wxHORIZONTAL);
        hs->Add(Icon(header, kSvgGradCap, kLpNavy, 20), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 28);
        hs->Add(Label(header, wxString::FromUTF8("Map\xC3\xBA" "a Campus Transport"), 9, kLpInk, true),
                0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        hs->Add(Badge(header, "Student portal", kLpStrip, kLpNavy), 0,
                wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        hs->AddStretchSpacer(1);
        hs->Add(Label(header, v.dateText, 8, kLpMuted), 0, wxALIGN_CENTER_VERTICAL);
        hs->Add(Icon(header, kSvgBell, kLpMuted, 18), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 22);

        auto* avatar = new RoundedPanel(header, kLpStrip, wxColour(), 18);
        avatar->SetMinSize(avatar->FromDIP(wxSize(36, 36)));
        auto* av = new wxBoxSizer(wxVERTICAL);
        av->AddStretchSpacer(1);
        av->Add(Label(avatar, v.initials, 8, kLpNavy, true), 0, wxALIGN_CENTER_HORIZONTAL);
        av->AddStretchSpacer(1);
        avatar->SetSizer(av);
        hs->Add(avatar, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);

        auto* who = new wxBoxSizer(wxVERTICAL);
        who->Add(Label(header, v.fullName, 8, kLpInk, true), 0, wxALIGN_RIGHT);
        who->Add(Label(header, v.studentId, 7, kLpMuted), 0, wxALIGN_RIGHT | wxTOP, 1);
        hs->Add(who, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);

        auto* hv = new wxBoxSizer(wxVERTICAL);
        hv->AddSpacer(14);
        hv->Add(hs, 0, wxEXPAND);
        hv->AddSpacer(14);
        header->SetSizer(hv);
        rs->Add(header, 0, wxEXPAND);
        rs->Add(Rule(right), 0, wxEXPAND);
    }

    // ------------------------------------------------------- scrolling body
    auto* content = new wxScrolledWindow(right, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxVSCROLL | wxBORDER_NONE);
    content->SetScrollRate(0, 10);
    content->SetBackgroundColour(kLpCanvas);
    auto* outer = new wxBoxSizer(wxVERTICAL);
    auto* pad = new wxBoxSizer(wxVERTICAL);
    outer->Add(pad, 1, wxEXPAND | wxALL, 26);

    // --- title row
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* titles = new wxBoxSizer(wxVERTICAL);
        titles->Add(Label(content, "RESERVE A RIDE / " + v.routeName.Upper(), 7, kLpRed, true));
        titles->Add(Label(content, wxString::FromUTF8("You\xE2\x80\x99re all set, ") + v.firstName + ".",
                          22, kLpInk, true), 0, wxTOP, 6);
        titles->Add(Label(content,
                          wxString::FromUTF8("Your seat is reserved and your payment is confirmed. Here\xE2\x80\x99s everything for your ride."),
                          9, kLpMuted), 0, wxTOP, 6);
        row->Add(titles, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(new PillButton(content, "Back to trips  " + ArrowLeft(), *wxWHITE, kLpNavy, kLpBorder, 9,
                                [this] { ShowPage(Page::Reservations); }),
                 0, wxALIGN_TOP);
        pad->Add(row, 0, wxEXPAND);
    }

    // --- stepper
    {
        pad->AddSpacer(20);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        const wxString steps[] = {"Destination", "Route", "Shuttle / Driver", "Seat selector", "Payment", "Confirmation"};
        for (int i = 0; i < 6; ++i) {
            const bool active = (i == 5);
            auto* circle = new RoundedPanel(content, active ? kLpNavy : kLpGreenBg, wxColour(), 11);
            circle->SetMinSize(circle->FromDIP(wxSize(22, 22)));
            auto* cs = new wxBoxSizer(wxVERTICAL);
            cs->AddStretchSpacer(1);
            if (active) cs->Add(Label(circle, "6", 8, *wxWHITE, true), 0, wxALIGN_CENTER_HORIZONTAL);
            else        cs->Add(Icon(circle, kSvgCheck, kLpGreen, 12), 0, wxALIGN_CENTER_HORIZONTAL);
            cs->AddStretchSpacer(1);
            circle->SetSizer(cs);
            row->Add(circle, 0, wxALIGN_CENTER_VERTICAL);
            row->Add(Label(content, steps[i], 8, active ? kLpNavy : kLpMuted, active), 0,
                     wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
            if (i < 5) {
                auto* line = new wxPanel(content, wxID_ANY, wxDefaultPosition, wxSize(16, 1));
                line->SetBackgroundColour(kLpBorder);
                row->Add(line, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
            }
        }
        pad->Add(row, 0, wxEXPAND);
    }

    // --- success banner
    {
        pad->AddSpacer(20);
        auto* banner = new RoundedPanel(content, kLpGreenBg, kLpGreenLine, 8);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Icon(banner, kSvgCheckCircle, kLpGreen, 24), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 18);
        auto* text = new wxBoxSizer(wxVERTICAL);
        text->Add(Label(banner, "Reservation confirmed " + MidDot() + " Payment received", 9, kLpGreen, true));
        text->Add(Label(banner,
                        "Booking " + v.bookingId + " " + MidDot() +
                        wxString::Format(" A digital pass has been issued for seat %d.", v.seatNo),
                        8, kLpGreen), 0, wxTOP, 2);
        row->Add(text, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        row->Add(Badge(banner, "Confirmed", kLpStrip, kLpNavy), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 18);
        auto* bv = new wxBoxSizer(wxVERTICAL);
        bv->AddSpacer(14);
        bv->Add(row, 0, wxEXPAND);
        bv->AddSpacer(14);
        banner->SetSizer(bv);
        pad->Add(banner, 0, wxEXPAND);
    }

    // --- two-column split
    pad->AddSpacer(18);
    auto* split = new wxBoxSizer(wxHORIZONTAL);

    // ----- left card: seat map
    {
        auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
        auto* cs = new wxBoxSizer(wxVERTICAL);
        auto* inner = new wxBoxSizer(wxVERTICAL);

        auto* head = new wxBoxSizer(wxHORIZONTAL);
        head->Add(Label(card, "Your seat on " + v.shuttleId, 13, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
        head->AddStretchSpacer(1);
        head->Add(Badge(card, wxString::Format("Seat %d reserved", v.seatNo), kLpStrip, kLpNavy), 0,
                  wxALIGN_CENTER_VERTICAL);
        inner->Add(head, 0, wxEXPAND);

        // legend
        auto* legend = new wxBoxSizer(wxHORIZONTAL);
        const struct { wxColour fill, border; const char* text; } keys[] = {
            {*wxWHITE, kLpBorder, "Available"},
            {kLpSeatTaken, kLpBorder, "Occupied"},
            {kLpNavy, kLpNavy, "Your seat"},
        };
        for (const auto& k : keys) {
            legend->Add(Swatch(card, k.fill, k.border), 0, wxALIGN_CENTER_VERTICAL);
            legend->Add(Label(card, k.text, 7, kLpMuted), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
            legend->AddSpacer(8);
        }
        inner->Add(legend, 0, wxTOP, 12);

        auto* body = new wxBoxSizer(wxHORIZONTAL);

        // seat map
        auto* map = new RoundedPanel(card, kLpSoft, kLpBorder, 28);
        auto* ms = new wxBoxSizer(wxVERTICAL);
        auto* top = new wxBoxSizer(wxHORIZONTAL);
        top->Add(Label(map, "FRONT", 6, kLpMuted, true));
        top->AddStretchSpacer(1);
        top->Add(Label(map, "DRIVER", 6, kLpMuted, true));
        ms->AddSpacer(18);
        ms->Add(top, 0, wxEXPAND | wxLEFT | wxRIGHT, 18);
        ms->AddSpacer(8);
        ms->Add(Rule(map), 0, wxEXPAND | wxLEFT | wxRIGHT, 18);
        ms->AddSpacer(14);

        auto* grid = new wxFlexGridSizer(4, 8, 8);     // 4 columns x 5 rows
        for (int n = 1; n <= v.seatCount; ++n) {
            SeatState st = SeatState::Available;
            if (n <= v.occupiedBefore) st = SeatState::Occupied;
            if (n == v.seatNo)         st = SeatState::Mine;
            const int col = (n - 1) % 4;
            grid->Add(new SeatTile(map, n, st), 0, col == 2 ? wxLEFT : 0, 22);   // aisle gap
        }
        ms->Add(grid, 0, wxLEFT | wxRIGHT, 18);
        ms->AddSpacer(10);
        ms->Add(Label(map, wxString::Format("REAR %s %d-SEAT SHUTTLE", MidDot(), v.seatCount), 6, kLpMuted),
                0, wxALIGN_CENTER_HORIZONTAL);
        ms->AddSpacer(12);
        map->SetSizer(ms);
        body->Add(map, 0, wxALIGN_TOP);

        // trip details
        auto* details = new wxBoxSizer(wxVERTICAL);
        const struct { wxString caption, value; } rows[] = {
            {"Destination", v.destination},
            {"Departure " + MidDot() + " " + v.departDate, v.departTime + " " + MidDot() + " " + v.bay},
            {"Route", v.routeName + " " + MidDot() + " " + v.routeCode},
            {"Assigned driver", v.driver + " " + MidDot() + " " + v.shuttleId},
        };
        for (const auto& r : rows) {
            details->Add(Label(card, r.caption, 7, kLpMuted));
            details->Add(Label(card, r.value, 10, kLpInk, true), 0, wxTOP | wxBOTTOM, 4);
            details->AddSpacer(12);
        }
        auto* note = new RoundedPanel(card, kLpStrip, wxColour(), 6);
        auto* ns = new wxBoxSizer(wxVERTICAL);
        ns->Add(Label(note, wxString::Format("Seat %d %s %s", v.seatNo, MidDot(), v.seatNote), 8, kLpNavy, true),
                0, wxLEFT | wxRIGHT | wxTOP, 12);
        ns->Add(Label(note, wxString::Format("%d occupied before booking", v.occupiedBefore), 7, kLpMuted),
                0, wxLEFT | wxRIGHT | wxTOP, 12);
        ns->Add(Label(note, wxString::Format("%d/%d reserved after your booking", v.occupiedBefore + 1, v.seatCount),
                      7, kLpMuted), 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 12);
        ns->GetItem(size_t(1))->SetBorder(4);
        ns->GetItem(size_t(2))->SetBorder(2);
        note->SetSizer(ns);
        details->Add(note, 0, wxEXPAND);
        body->Add(details, 1, wxEXPAND | wxLEFT, 24);

        inner->Add(body, 1, wxEXPAND | wxTOP, 14);
        cs->Add(inner, 1, wxEXPAND | wxALL, 20);
        card->SetSizer(cs);
        split->Add(card, 3, wxEXPAND);
    }

    split->AddSpacer(18);

    // ----- right card: payment summary
    {
        auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
        auto* cs = new wxBoxSizer(wxVERTICAL);
        auto* inner = new wxBoxSizer(wxVERTICAL);

        auto* head = new wxBoxSizer(wxHORIZONTAL);
        head->Add(Label(card, "Payment summary", 13, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
        head->AddStretchSpacer(1);
        head->Add(Badge(card, "Paid", kLpGreenBg, kLpGreen), 0, wxALIGN_CENTER_VERTICAL);
        inner->Add(head, 0, wxEXPAND);

        inner->Add(Label(card, "Passenger " + MidDot() + " Student ID", 7, kLpMuted), 0, wxTOP, 16);
        inner->Add(Label(card, v.fullName + " " + MidDot() + " " + v.studentId, 10, kLpInk, true), 0, wxTOP, 4);
        inner->AddSpacer(14);
        inner->Add(Rule(card), 0, wxEXPAND);
        inner->AddSpacer(14);

        auto* fares = new wxFlexGridSizer(2, 8, 12);
        fares->AddGrowableCol(0);
        fares->Add(Label(card, v.routeName + " fare", 8, kLpMuted));
        fares->Add(Label(card, Money(v.fare), 8, kLpInk, true), 0, wxALIGN_RIGHT);
        fares->Add(Label(card, "Booking fee", 8, kLpMuted));
        fares->Add(Label(card, Money(v.fee), 8, kLpInk, true), 0, wxALIGN_RIGHT);
        inner->Add(fares, 0, wxEXPAND);

        auto* total = new wxBoxSizer(wxHORIZONTAL);
        total->Add(Label(card, "Total paid", 9, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
        total->AddStretchSpacer(1);
        total->Add(Label(card, Money(v.fare + v.fee), 20, kLpNavy, true), 0, wxALIGN_CENTER_VERTICAL);
        inner->Add(total, 0, wxEXPAND | wxTOP, 14);

        // receipt box
        auto* receipt = new RoundedPanel(card, kLpCanvas, wxColour(), 8);
        auto* rrow = new wxBoxSizer(wxHORIZONTAL);
        auto* iconBox = new RoundedPanel(receipt, kLpStrip, wxColour(), 6);
        auto* ib = new wxBoxSizer(wxVERTICAL);
        ib->Add(Icon(iconBox, kSvgWallet, kLpNavy, 16), 0, wxALL, 7);
        iconBox->SetSizer(ib);
        rrow->Add(iconBox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        auto* rtext = new wxBoxSizer(wxVERTICAL);
        rtext->Add(Label(receipt, v.payMethod + " " + Bullets() + " " + v.payLast4, 8, kLpInk, true));
        rtext->Add(Label(receipt, "Ref. " + v.payRef + " " + MidDot() + " Paid today at " + v.paidAt, 7, kLpMuted),
                   0, wxTOP, 3);
        rrow->Add(rtext, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        auto* rv = new wxBoxSizer(wxVERTICAL);
        rv->AddSpacer(12);
        rv->Add(rrow, 0, wxEXPAND);
        rv->AddSpacer(12);
        receipt->SetSizer(rv);
        inner->Add(receipt, 0, wxEXPAND | wxTOP, 18);

        // primary action
        inner->Add(new PillButton(card, "View my digital pass", kLpNavy, *wxWHITE, wxColour(), 10,
                                  [this] { ShowPage(Page::DigitalPass); },
                                  SvgBundle(kSvgQr, *wxWHITE, 16), 14),
                   0, wxEXPAND | wxTOP, 14);
        inner->Add(Link(card, "Download receipt  " + ArrowDown(), 8, kLpNavy, true,
                        [] { /* TODO: export receipt */ }),
                   0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 14);
        auto* policy = Label(card,
                             "Need to cancel? Review the cancellation policy in My Reservations before your departure.",
                             7, kLpMuted);
        policy->Wrap(card->FromDIP(320));
        inner->Add(policy, 0, wxTOP, 14);

        cs->Add(inner, 1, wxEXPAND | wxALL, 20);
        card->SetSizer(cs);
        split->Add(card, 2, wxEXPAND);
    }
    pad->Add(split, 0, wxEXPAND);

    // --- footer route tracker
    {
        pad->AddSpacer(18);
        auto* bar = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* left = new wxBoxSizer(wxVERTICAL);

        auto* stops = new wxBoxSizer(wxHORIZONTAL);
        for (size_t i = 0; i < v.stops.size(); ++i) {
            const bool last = (i + 1 == v.stops.size());
            stops->Add(Dot(bar, last ? kLpRed : kLpNavy, 8), 0, wxALIGN_CENTER_VERTICAL);
            stops->Add(Label(bar, v.stops[i], 8, kLpInk, last), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
            if (!last)
                stops->Add(Icon(bar, kSvgChevron, kLpMuted, 12), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 12);
        }
        left->Add(stops, 0);
        left->Add(Label(bar,
                        "Arrive at " + v.bay + " by " + v.arriveBy + ". Have your digital pass and student ID ready.",
                        8, kLpMuted), 0, wxTOP, 8);
        row->Add(left, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 18);
        row->Add(new PillButton(bar, "Boarding guidelines", *wxWHITE, kLpNavy, kLpBorder, 8,
                                [] { /* TODO: show boarding guidelines dialog */ }),
                 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 18);
        auto* bv = new wxBoxSizer(wxVERTICAL);
        bv->AddSpacer(14);
        bv->Add(row, 0, wxEXPAND);
        bv->AddSpacer(14);
        bar->SetSizer(bv);
        pad->Add(bar, 0, wxEXPAND);
    }

    content->SetSizer(outer);
    content->FitInside();
    rs->Add(content, 1, wxEXPAND);
    right->SetSizer(rs);
    shellSizer->Add(right, 1, wxEXPAND);
    shell->SetSizer(shellSizer);
    return shell;
}