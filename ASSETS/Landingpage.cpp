// LandingPage.cpp  -  TaraMapuan landing page (matches the web mock-up)
// Requires wxWidgets 3.2+ (wxBitmapBundle::FromSVG for the icons).
//
// Add to MainFrame.h:   wxScrolledWindow* BuildLandingPage();
// Add this file to your CMake / project sources.

#include "MainFrame.h"

#include <wx/bmpbndl.h>
#include <wx/dcbuffer.h>
#include <wx/statbmp.h>

#include <functional>
#include <string>

namespace {

// ---- palette -------------------------------------------------------------
const wxColour kLpNavy(15, 42, 82);
const wxColour kLpRed(190, 24, 58);
const wxColour kLpInk(24, 38, 64);
const wxColour kLpMuted(106, 118, 140);
const wxColour kLpStrip(232, 239, 250);
const wxColour kLpGreen(22, 128, 92);
const wxColour kLpBorder(222, 229, 240);
const wxColour kLpSoft(247, 249, 252);
const wxColour kLpPink(252, 232, 237);
const wxColour kLpHeroSub(214, 223, 238);

// ---- glyphs (UTF-8 escapes so the source encoding never matters) ----------
wxString ArrowUp()    { return wxString::FromUTF8("\xE2\x86\x97"); }  // ↗
wxString ArrowRight() { return wxString::FromUTF8("\xE2\x86\x92"); }  // →
wxString Dot()        { return wxString::FromUTF8("\xC2\xB7"); }      // ·

// ---- SVG icons -----------------------------------------------------------
const char* kSvgBus = R"(<rect x='4' y='3' width='16' height='15' rx='3'/><path d='M4 11h16M8 18v2M16 18v2'/><circle cx='8' cy='14.5' r='.6'/><circle cx='16' cy='14.5' r='.6'/>)";
const char* kSvgPin = R"(<path d='M12 21s-6.5-5.6-6.5-11a6.5 6.5 0 0 1 13 0c0 5.4-6.5 11-6.5 11z'/><circle cx='12' cy='10' r='2.3'/>)";
const char* kSvgBuilding = R"(<rect x='5' y='3' width='14' height='18' rx='1'/><path d='M9 7h2M13 7h2M9 11h2M13 11h2M10 21v-4h4v4'/>)";
const char* kSvgHome = R"(<path d='M4 11l8-7 8 7'/><path d='M6 10v10h12V10M10 20v-5h4v5'/>)";
const char* kSvgSignal = R"(<path d='M5 12a10 10 0 0 1 14 0M8 15a6 6 0 0 1 8 0'/><circle cx='12' cy='18' r='1'/>)";

wxStaticBitmap* Icon(wxWindow* parent, const char* body, const wxColour& stroke, int size) {
    const std::string svg =
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='" +
        stroke.GetAsString(wxC2S_HTML_SYNTAX).ToStdString() +
        "' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>" + body + "</svg>";
    const wxSize px = parent->FromDIP(wxSize(size, size));
    return new wxStaticBitmap(parent, wxID_ANY, wxBitmapBundle::FromSVG(svg.c_str(), px));
}

// ---- small widgets --------------------------------------------------------
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

// Rounded rectangle panel (cards, logo tile, step badges).
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

// Filled / outlined pill button with hover state.
class PillButton : public wxPanel {
public:
    PillButton(wxWindow* parent, const wxString& label, const wxColour& fill,
               const wxColour& text, const wxColour& border, int pt,
               std::function<void()> onClick)
        : wxPanel(parent, wxID_ANY), m_label(label), m_fill(fill), m_text(text), m_border(border) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetFont(wxFont(wxFontInfo(pt).Bold()));
        SetCursor(wxCursor(wxCURSOR_HAND));
        wxClientDC measure(this);
        measure.SetFont(GetFont());
        const wxSize t = measure.GetTextExtent(label);
        SetMinSize(wxSize(t.x + FromDIP(32), t.y + FromDIP(20)));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
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
            dc.DrawText(m_label, (s.x - t.x) / 2, (s.y - t.y) / 2);
        });
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hover = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hover = false; Refresh(); });
        Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
    }
private:
    wxString m_label;
    wxColour m_fill, m_text, m_border;
    bool m_hover = false;
};

void MakeClickable(wxWindow* w, const std::function<void()>& onClick) {
    w->SetCursor(wxCursor(wxCURSOR_HAND));
    w->Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
    for (wxWindow* child : w->GetChildren()) MakeClickable(child, onClick);
}

}  // namespace

wxScrolledWindow* MainFrame::BuildLandingPage() {
    auto* page = new wxScrolledWindow(m_pageHost, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxVSCROLL | wxBORDER_NONE);
    page->SetScrollRate(0, 10);
    page->SetBackgroundColour(*wxWHITE);
    auto* col = new wxBoxSizer(wxVERTICAL);

    const int padX = 52;

    auto toPortal = [this] { ShowPage(Page::RoleSelect); };
    auto toHome   = [this] { ShowPage(Page::Landing); };
    auto toDriver = [this] { m_selectedRole = UserRole::Driver; ShowPage(Page::Login); };

    // Creates a full-width coloured band and returns {panel, inner sizer with side padding}.
    auto band = [&](const wxColour& bg, int padY) {
        auto* panel = new wxPanel(page, wxID_ANY);
        panel->SetBackgroundColour(bg);
        auto* outer = new wxBoxSizer(wxVERTICAL);
        auto* inner = new wxBoxSizer(wxHORIZONTAL);
        outer->AddSpacer(padY);
        outer->Add(inner, 0, wxEXPAND | wxLEFT | wxRIGHT, padX);
        outer->AddSpacer(padY);
        panel->SetSizer(outer);
        col->Add(panel, 0, wxEXPAND);
        return std::make_pair(panel, inner);
    };

    // ---------------------------------------------------------------- navbar
    {
        auto [nav, row] = band(*wxWHITE, 14);

        auto* logo = new RoundedPanel(nav, kLpRed, wxColour(), 8);
        auto* logoSizer = new wxBoxSizer(wxVERTICAL);
        logoSizer->Add(Icon(logo, kSvgBus, *wxWHITE, 20), 0, wxALL, 8);
        logo->SetSizer(logoSizer);
        row->Add(logo, 0, wxALIGN_CENTER_VERTICAL);
        row->Add(Link(nav, "TaraMapuan", 15, kLpNavy, true, toHome), 0,
                 wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

        row->AddStretchSpacer(1);
        row->Add(Link(nav, "Home", 9, kLpNavy, true, toHome), 0, wxALIGN_CENTER_VERTICAL);
        for (const char* name : {"Routes", "Shuttle Schedule", "How It Works", "About"})
            row->Add(Link(nav, name, 9, kLpMuted, false, toPortal), 0,
                     wxALIGN_CENTER_VERTICAL | wxLEFT, 26);
        row->AddStretchSpacer(1);

        row->Add(new PillButton(nav, "Login", *wxWHITE, kLpNavy, kLpBorder, 9, toPortal), 0,
                 wxALIGN_CENTER_VERTICAL);
        row->Add(new PillButton(nav, "Reserve Seat  " + ArrowUp(), kLpRed, *wxWHITE, wxColour(), 9,
                                toPortal),
                 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    }

    // ------------------------------------------------------------ live strip
    {
        auto [strip, row] = band(kLpStrip, 12);

        row->Add(Icon(strip, kSvgSignal, kLpGreen, 18), 0, wxALIGN_CENTER_VERTICAL);
        auto* status = new wxBoxSizer(wxVERTICAL);
        status->Add(Label(strip, "On the move", 9, kLpNavy, true));
        status->Add(Label(strip, "Updated 7:12 AM " + Dot() + " Today", 7, kLpMuted), 0, wxTOP, 2);
        row->Add(status, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

        struct Trip { const char* route; const char* meta; const char* time; const char* state; };
        const Trip trips[] = {
            {"Campus Loop",       "SH-001 \xC2\xB7 Juan Reyes", "7:30 AM", "2 seats left"},
            {"Town Connector",    "SH-004 \xC2\xB7 Maria Cruz", "7:45 AM", "8 seats left"},
            {"Dormitory Express", "SH-008 \xC2\xB7 Paolo Lim",  "8:00 AM", "Boarding soon"},
        };
        for (const auto& t : trips) {
            auto* sep = new wxPanel(strip, wxID_ANY, wxDefaultPosition, wxSize(1, 34));
            sep->SetBackgroundColour(wxColour(205, 216, 235));
            row->Add(sep, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 18);

            auto* info = new wxBoxSizer(wxVERTICAL);
            info->Add(Label(strip, wxString::FromUTF8(t.route), 9, kLpNavy, true));
            info->Add(Label(strip, wxString::FromUTF8(t.meta), 7, kLpMuted), 0, wxTOP, 2);
            auto* when = new wxBoxSizer(wxVERTICAL);
            when->Add(Label(strip, t.time, 9, kLpNavy, true), 0, wxALIGN_RIGHT);
            when->Add(Label(strip, t.state, 7, kLpGreen), 0, wxALIGN_RIGHT | wxTOP, 2);

            auto* item = new wxBoxSizer(wxHORIZONTAL);
            item->Add(info, 0, wxALIGN_CENTER_VERTICAL);
            item->AddStretchSpacer(1);
            item->Add(when, 0, wxALIGN_CENTER_VERTICAL);
            row->Add(item, 1, wxALIGN_CENTER_VERTICAL);
        }
    }

    // ------------------------------------------------------------------ hero
    {
        auto [hero, row] = band(kLpNavy, 44);
        auto* stack = new wxBoxSizer(wxVERTICAL);

        auto* eyebrow = new wxBoxSizer(wxHORIZONTAL);
        auto* rule = new wxPanel(hero, wxID_ANY, wxDefaultPosition, wxSize(20, 2));
        rule->SetBackgroundColour(kLpRed);
        eyebrow->Add(rule, 0, wxALIGN_CENTER_VERTICAL);
        eyebrow->Add(Label(hero, "CAMPUS CONNECTED. COMMUTES SIMPLIFIED.", 7, kLpHeroSub, true),
                     0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        stack->Add(eyebrow, 0, wxBOTTOM, 14);

        stack->Add(Label(hero, "Your Route.\nYour Shuttle.\nYour Ride.", 38, *wxWHITE, true));
        stack->Add(Label(hero,
                         "From your first class to your last stop. Find your route, choose a shuttle,\n"
                         "and save your seat with TaraMapuan.",
                         10, kLpHeroSub),
                   0, wxTOP, 18);

        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        actions->Add(new PillButton(hero, "Find my route  " + ArrowRight(), kLpRed, *wxWHITE,
                                    wxColour(), 9, toPortal),
                     0, wxALIGN_CENTER_VERTICAL);
        actions->Add(Link(hero, "View shuttle schedule  " + ArrowUp(), 9, *wxWHITE, true, toPortal),
                     0, wxALIGN_CENTER_VERTICAL | wxLEFT, 18);
        stack->Add(actions, 0, wxTOP, 22);
        stack->AddSpacer(30);

        row->Add(stack, 1);
    }

    // ------------------------------------------- destinations + three steps
    {
        auto [section, row] = band(*wxWHITE, 22);

        // left column
        auto* left = new wxBoxSizer(wxVERTICAL);
        auto* head = new wxBoxSizer(wxHORIZONTAL);
        head->Add(Label(section, "Where are you headed?", 15, kLpInk, true), 0,
                  wxALIGN_CENTER_VERTICAL);
        head->AddStretchSpacer(1);
        head->Add(Link(section, "Explore routes " + ArrowRight(), 8, kLpNavy, true, toPortal), 0,
                  wxALIGN_CENTER_VERTICAL);
        left->Add(head, 0, wxEXPAND);
        left->Add(Label(section, "Your destination comes first. We'll show the shuttles that get you there.",
                        8, kLpMuted),
                  0, wxTOP | wxBOTTOM, 10);

        struct Dest { const char* name; const char* route; const char* svg; };
        const Dest dests[] = {
            {"Main Gate",   "Campus Loop",       kSvgPin},
            {"Town Center", "Town Connector",    kSvgBuilding},
            {"Dormitory",   "Dormitory Express", kSvgHome},
            {"Terminal",    "Terminal Link",     kSvgBus},
        };
        auto* cards = new wxBoxSizer(wxHORIZONTAL);
        bool first = true;
        for (const auto& d : dests) {
            auto* card = new RoundedPanel(section, kLpSoft, kLpBorder, 8);
            auto* cs = new wxBoxSizer(wxVERTICAL);
            cs->Add(Icon(card, d.svg, kLpNavy, 18), 0, wxLEFT | wxRIGHT | wxTOP, 14);
            cs->Add(Label(card, d.name, 10, kLpInk, true), 0, wxLEFT | wxRIGHT | wxTOP, 14);
            cs->Add(Label(card, d.route, 8, kLpMuted), 0, wxLEFT | wxRIGHT | wxTOP, 8);
            cs->Add(Label(card, "View departures " + ArrowUp(), 8, kLpNavy, true), 0,
                    wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 14);
            card->SetSizer(cs);
            for (wxWindow* c : card->GetChildren()) c->SetBackgroundColour(kLpSoft);
            MakeClickable(card, toPortal);
            cards->Add(card, 1, wxEXPAND | (first ? 0 : wxLEFT), 10);
            first = false;
        }
        left->Add(cards, 0, wxEXPAND);
        row->Add(left, 3, wxEXPAND);

        // right column
        auto* right = new wxBoxSizer(wxVERTICAL);
        right->Add(Label(section, "A better ride in three steps.", 15, kLpInk, true));
        right->AddSpacer(14);

        struct Step { const char* n; const char* title; const char* desc; };
        const Step steps[] = {
            {"01", "Find your route",     "Choose a destination and a departure."},
            {"02", "Make it your seat",   "Pick a seat and pay securely."},
            {"03", "Scan. Board. Go.",    "Show your digital pass to your driver."},
        };
        for (const auto& s : steps) {
            auto* line = new wxBoxSizer(wxHORIZONTAL);
            auto* badge = new RoundedPanel(section, kLpPink, wxColour(), 6);
            auto* bs = new wxBoxSizer(wxVERTICAL);
            bs->Add(Label(badge, s.n, 8, kLpRed, true), 0, wxALL, 7);
            badge->SetSizer(bs);
            for (wxWindow* c : badge->GetChildren()) c->SetBackgroundColour(kLpPink);
            line->Add(badge, 0, wxALIGN_TOP);

            auto* text = new wxBoxSizer(wxVERTICAL);
            text->Add(Label(section, s.title, 9, kLpInk, true));
            text->Add(Label(section, s.desc, 8, kLpMuted), 0, wxTOP, 3);
            line->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
            right->Add(line, 0, wxEXPAND | wxBOTTOM, 14);
        }
        row->AddSpacer(36);
        row->Add(right, 2, wxEXPAND);
    }

    // ---------------------------------------------------------------- footer
    {
        auto [footer, row] = band(kLpSoft, 20);

        auto* left = new wxBoxSizer(wxVERTICAL);
        left->Add(Label(footer, wxString::FromUTF8("Built for the Map\xC3\xBA" "an on the move."), 9,
                        kLpInk, true));
        left->Add(Label(footer,
                        wxString::FromUTF8("A connected, reliable campus shuttle experience. \xC2\xA9 2026 TaraMapuan"),
                        8, kLpMuted),
                  0, wxTOP, 4);
        row->Add(left, 0, wxALIGN_CENTER_VERTICAL);
        row->AddStretchSpacer(1);
        row->Add(Link(footer, "Transport help desk", 8, kLpNavy, false, toPortal), 0,
                 wxALIGN_CENTER_VERTICAL);
        row->Add(Link(footer, "Privacy policy", 8, kLpNavy, false, toPortal), 0,
                 wxALIGN_CENTER_VERTICAL | wxLEFT, 22);
        row->Add(Link(footer, "Driver login " + ArrowUp(), 8, kLpNavy, false, toDriver), 0,
                 wxALIGN_CENTER_VERTICAL | wxLEFT, 22);
    }

    page->SetSizer(col);
    page->FitInside();
    return page;
}