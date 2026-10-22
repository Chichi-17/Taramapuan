// DigitalPassPage.cpp  -  TaraMapuan | Passenger Portal: Digital Pass
// wxWidgets 3.2+.  Needs UiWidgets.h (shared widgets, palette, icons, shell).
//
// Add to MainFrame.h:   wxWindow* BuildDigitalPassPage();
//
// Connected here (as requested):
//   sidebar "Dashboard"                      -> ShowPage(Page::PassengerDashboard)
//   sidebar "My Reservations"                -> ShowPage(Page::ReservationConfirmation)
//   bottom card "My reservations" button     -> ShowPage(Page::ReservationConfirmation)
// Everything else is visual only (empty TODO handlers).

#include "MainFrame.h"
#include "UiWidgets.h"

#include <string>
#include <vector>

using namespace ui;

namespace {

// ---- icons specific to this view ---------------------------------------------
const char* const kSvgDownload     = R"(<path d='M12 4v11M7 11l5 5 5-5M5 20h14'/>)";
const char* const kSvgIdCard       = R"(<rect x='3' y='5' width='18' height='14' rx='2'/><circle cx='9' cy='11' r='2'/><path d='M6 16c.5-1.5 1.7-2 3-2s2.5.5 3 2M15 10h3M15 14h3'/>)";
const char* const kSvgClock        = R"(<circle cx='12' cy='12' r='9'/><path d='M12 7v5l3 2'/>)";
const char* const kSvgPin          = R"(<path d='M12 21s-6.5-5.6-6.5-11a6.5 6.5 0 0 1 13 0c0 5.4-6.5 11-6.5 11z'/><circle cx='12' cy='10' r='2.3'/>)";
const char* const kSvgTicket       = R"(<path d='M3 9V7a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2v2a2.5 2.5 0 0 0 0 5v3a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-3a2.5 2.5 0 0 0 0-5z'/><path d='M9 8v8'/>)";
const char* const kSvgChevronRight = R"(<path d='M9 6l6 6-6 6'/>)";

const wxColour kBannerFill(254, 243, 214);
const wxColour kBannerLine(247, 225, 170);

// ---- panel whose fill has rounded corners on one side only -------------------
// Used for the navy header / grey footer of the ticket so they follow the
// card's rounded outline.
class BandPanel : public wxPanel {
public:
    enum class Round { Top, Bottom };
    BandPanel(wxWindow* parent, const wxColour& fill, int radius, Round which)
        : wxPanel(parent, wxID_ANY), m_fill(fill), m_radius(radius), m_which(which) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(fill);
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
            dc.Clear();
            dc.SetBrush(wxBrush(m_fill));
            dc.SetPen(*wxTRANSPARENT_PEN);
            const wxSize s = GetClientSize();
            const int r = m_radius;
            if (m_which == Round::Top) dc.DrawRoundedRectangle(0, 0, s.x, s.y + 2 * r, r);
            else                       dc.DrawRoundedRectangle(0, -2 * r, s.x, s.y + 2 * r, r);
        });
    }
private:
    wxColour m_fill;
    int m_radius;
    Round m_which;
};

// ---- dashed tear-off line ----------------------------------------------------
class DashedLine : public wxPanel {
public:
    DashedLine(wxWindow* parent, const wxColour& colour) : wxPanel(parent, wxID_ANY), m_colour(colour) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(-1, 1));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
            dc.Clear();
            dc.SetPen(wxPen(m_colour, 1, wxPENSTYLE_SHORT_DASH));
            dc.DrawLine(0, 0, GetClientSize().x, 0);
        });
    }
private:
    wxColour m_colour;
};

// ---- placeholder QR code (drawn, deterministic) --------------------------------
class QrPlaceholder : public wxPanel {
public:
    explicit QrPlaceholder(wxWindow* parent, const wxColour& ink = kLpNavy)
        : wxPanel(parent, wxID_ANY), m_ink(ink), m_cells(N * N, false) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(FromDIP(wxSize(170, 170)));

        unsigned seed = 0x9E3779B1u;
        for (int i = 0; i < N * N; ++i) {
            seed = seed * 1664525u + 1013904223u;
            m_cells[i] = ((seed >> 16) & 0xFF) < 118;
        }
        const int corners[3][2] = {{0, 0}, {N - 7, 0}, {0, N - 7}};
        for (const auto& c : corners) {
            for (int dy = -1; dy <= 7; ++dy)             // quiet zone around each finder
                for (int dx = -1; dx <= 7; ++dx) {
                    const int x = c[0] + dx, y = c[1] + dy;
                    if (x >= 0 && x < N && y >= 0 && y < N) m_cells[y * N + x] = false;
                }
            for (int dy = 0; dy < 7; ++dy)
                for (int dx = 0; dx < 7; ++dx) {
                    const bool ring = dx == 0 || dx == 6 || dy == 0 || dy == 6;
                    const bool core = dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4;
                    m_cells[(c[1] + dy) * N + (c[0] + dx)] = ring || core;
                }
        }
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
            dc.Clear();
            const wxSize s = GetClientSize();
            const int cell = std::max(1, std::min(s.x, s.y) / N);
            const int ox = (s.x - cell * N) / 2, oy = (s.y - cell * N) / 2;
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(m_ink));
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x)
                    if (m_cells[y * N + x]) dc.DrawRectangle(ox + x * cell, oy + y * cell, cell, cell);
        });
    }
private:
    static constexpr int N = 25;
    wxColour m_ink;
    std::vector<bool> m_cells;
};

// ---- stylised live-map placeholder -------------------------------------------------
class MapPlaceholder : public wxPanel {
public:
    explicit MapPlaceholder(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(FromDIP(wxSize(120, 128)));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) { OnPaint(); });
    }
private:
    void OnPaint() {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        const wxSize s = GetClientSize();
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxColour(234, 240, 238)));
        dc.DrawRoundedRectangle(0, 0, s.x, s.y, 8);

        auto block = [&](double fx, double fy, double fw, double fh, const wxColour& c) {
            dc.SetBrush(wxBrush(c));
            dc.DrawRoundedRectangle(int(s.x * fx), int(s.y * fy), int(s.x * fw), int(s.y * fh), 6);
        };
        const wxColour grey(214, 222, 232), green(208, 230, 214);
        block(0.04, 0.10, 0.14, 0.16, grey);
        block(0.33, 0.10, 0.14, 0.14, grey);
        block(0.58, 0.08, 0.06, 0.12, grey);
        block(0.82, 0.10, 0.15, 0.16, grey);
        block(0.10, 0.36, 0.11, 0.10, green);
        block(0.68, 0.74, 0.20, 0.10, green);
        block(0.38, 0.84, 0.14, 0.10, grey);
        block(0.56, 0.84, 0.10, 0.10, grey);

        struct Stop { double fx, fy; const char* name; bool above; };
        const Stop stops[] = {
            {0.13, 0.70, "Campus", false},
            {0.34, 0.47, "Library", true},
            {0.62, 0.64, "Student Center", false},
            {0.86, 0.42, "Main Gate", true},
        };
        wxPoint pts[4];
        for (int i = 0; i < 4; ++i) pts[i] = wxPoint(int(s.x * stops[i].fx), int(s.y * stops[i].fy));
        dc.SetPen(wxPen(kLpNavy, 2));
        dc.DrawLines(4, pts);
        dc.SetFont(wxFont(wxFontInfo(6).Bold()));
        dc.SetTextForeground(kLpInk);
        for (int i = 0; i < 4; ++i) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(i == 3 ? kLpRed : kLpNavy));
            dc.DrawCircle(pts[i], 4);
            const wxSize t = dc.GetTextExtent(stops[i].name);
            int tx = pts[i].x - t.x / 2;
            tx = std::max(4, std::min(tx, s.x - t.x - 4));
            dc.DrawText(stops[i].name, tx, stops[i].above ? pts[i].y - t.y - 6 : pts[i].y + 7);
        }
    }
};

// ---- sample data (replace with a real ReservationRecord later) ------------------
struct PassView {
    wxString fullName   = "Alex Santos";
    wxString studentId  = "2024-001234";
    wxString initials   = "AS";
    wxString dateText   = "Thu, 08 Oct 2026";
    wxString from       = "Campus";
    wxString to         = "Main Gate";
    int      seatNo     = 19;
    wxString shuttle    = "SH-001";
    wxString driver     = "Juan Reyes";
    wxString departure  = "7:30 AM";
    wxString passDate   = "08 Oct 2026";
    wxString routeName  = "Campus Loop";
    wxString routeCode  = "R01";
    wxString seatNote   = "Aisle seat";
    wxString rideTime   = "12 min ride";
    wxString bookingId  = "TM-261008-0019";
    wxString fare       = Peso() + "20.00";
    wxString bay        = "Campus Transport Bay A";
    wxString updatedAt  = "7:12 AM";
    wxString opensAt    = "7:20 AM";
    wxString countdown  = "18 min to departure";
    std::vector<wxString> stops = {"Campus", "Library", "Student Center", "Main Gate"};
};

}  // namespace

wxWindow* MainFrame::BuildDigitalPassPage() {
    const PassView v;

    auto toDashboard    = [this] { ShowPage(Page::PassengerDashboard); };
    auto toReservations = [this] { ShowPage(Page::ReservationConfirmation); };

    auto* shell = new wxPanel(m_pageHost, wxID_ANY);
    shell->SetBackgroundColour(kLpCanvas);
    auto* shellSizer = new wxBoxSizer(wxHORIZONTAL);

    // =========================================================== SIDEBAR
    const std::vector<NavEntry> nav = {
        {"Dashboard",       kSvgDashboard, toDashboard,    false},     // connected
        {"Find Route",      kSvgRoute,     [] {},          false},
        {"Schedule",        kSvgCalendar,  [] {},          false},
        {"My Reservations", kSvgTicket,    toReservations, false},     // connected
        {"Digital Pass",    kSvgQr,        [] {},          true},
        {"Profile",         kSvgUser,      [] {},          false},
    };
    shellSizer->Add(MakeSidebar(shell, "PASSENGER PORTAL", nav, [] { /* TODO: logout */ }), 0, wxEXPAND);

    // ======================================================== RIGHT COLUMN
    auto* right = new wxPanel(shell, wxID_ANY);
    right->SetBackgroundColour(kLpCanvas);
    auto* rs = new wxBoxSizer(wxVERTICAL);
    rs->Add(MakeTopHeader(right, "Student portal", kLpGreyBadge, kLpMuted, v.dateText, v.initials,
                          v.fullName, v.studentId),
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
        titles->Add(Label(content, "MY RESERVATIONS / DIGITAL PASS", 7, kLpRed, true));
        titles->Add(Label(content, "Your boarding pass.", 22, kLpInk, true), 0, wxTOP, 6);
        titles->Add(Label(content, "One pass. One reserved seat. Show this to your driver when you board.",
                          9, kLpMuted), 0, wxTOP, 6);
        row->Add(titles, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(new PillButton(content, "Download pass", *wxWHITE, kLpNavy, kLpBorder, 9,
                                [] { /* TODO: export pass */ },
                                SvgBundle(kSvgDownload, kLpNavy, 16), 11),
                 0, wxALIGN_TOP);
        pad->Add(row, 0, wxEXPAND);
    }

    // ------------------------------------------------------------ alert banner
    {
        pad->AddSpacer(18);
        auto* banner = new RoundedPanel(content, kBannerFill, kBannerLine, 8);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Icon(banner, kSvgClock, kLpAmber, 18), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
        row->Add(Label(banner, "Boarding opens at " + v.opensAt + ". Head to " + v.bay + ".", 8, kLpAmber),
                 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        row->Add(Label(banner, v.countdown, 7, kLpAmber, true), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
        auto* bv = new wxBoxSizer(wxVERTICAL);
        bv->AddSpacer(14);
        bv->Add(row, 0, wxEXPAND);
        bv->AddSpacer(14);
        banner->SetSizer(bv);
        pad->Add(banner, 0, wxEXPAND);
    }

    // ------------------------------------------------------------ two columns
    pad->AddSpacer(18);
    auto* split = new wxBoxSizer(wxHORIZONTAL);

    // ===================== left: the digital pass (ticket)
    {
        auto* ticket = new RoundedPanel(content, *wxWHITE, kLpBorder, 12);
        auto* ts = new wxBoxSizer(wxVERTICAL);

        // --- navy header band
        {
            auto* band = new BandPanel(ticket, kLpNavy, 11, BandPanel::Round::Top);
            auto* row = new wxBoxSizer(wxHORIZONTAL);
            auto* left = new wxBoxSizer(wxVERTICAL);
            left->Add(Label(band, "TARAMAPUAN / DIGITAL BOARDING PASS", 6, kLpNavMuted));
            left->Add(Label(band, v.from + " " + ArrowRight() + " " + v.to, 17, *wxWHITE, true), 0, wxTOP, 8);
            row->Add(left, 1, wxALIGN_CENTER_VERTICAL);
            auto* seat = new wxBoxSizer(wxVERTICAL);
            seat->Add(Label(band, "YOUR SEAT", 6, kLpNavMuted), 0, wxALIGN_RIGHT);
            seat->Add(Label(band, wxString::Format("%d", v.seatNo), 30, *wxWHITE, true), 0, wxALIGN_RIGHT);
            row->Add(seat, 0, wxALIGN_CENTER_VERTICAL);
            auto* bv = new wxBoxSizer(wxVERTICAL);
            bv->AddSpacer(20);
            bv->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 26);
            bv->AddSpacer(20);
            band->SetSizer(bv);
            ts->Add(band, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 1);
        }

        // --- body: QR + passenger details
        {
            auto* body = new wxPanel(ticket, wxID_ANY);
            body->SetBackgroundColour(*wxWHITE);
            auto* row = new wxBoxSizer(wxHORIZONTAL);

            auto* qrCol = new wxBoxSizer(wxVERTICAL);
            qrCol->Add(new QrPlaceholder(body), 0, wxALIGN_CENTER_HORIZONTAL);
            qrCol->Add(Label(body, "Scan at boarding", 7, kLpNavy, true), 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 12);
            qrCol->Add(Label(body, "Illustrative QR " + MidDot() + " static design concept", 6, kLpMuted), 0,
                       wxALIGN_CENTER_HORIZONTAL | wxTOP, 4);
            row->Add(qrCol, 0, wxALIGN_TOP);

            auto field = [&](wxWindow* p, const wxString& caption, const wxString& value) {
                auto* f = new wxBoxSizer(wxVERTICAL);
                f->Add(Label(p, caption, 7, kLpMuted));
                f->Add(Label(p, value, 10, kLpInk, true), 0, wxTOP, 5);
                return f;
            };
            auto* info = new wxBoxSizer(wxVERTICAL);
            info->Add(field(body, "Passenger name", v.fullName), 0, wxEXPAND);
            info->Add(field(body, "Student ID", v.studentId), 0, wxEXPAND | wxTOP, 16);

            auto* grid = new wxFlexGridSizer(2, 16, 24);     // 2 columns
            grid->AddGrowableCol(0, 1);
            grid->AddGrowableCol(1, 1);
            grid->Add(field(body, "Shuttle", v.shuttle), 0, wxEXPAND);
            grid->Add(field(body, "Driver", v.driver), 0, wxEXPAND);
            grid->Add(field(body, "Departure", v.departure), 0, wxEXPAND);
            grid->Add(field(body, "Date", v.passDate), 0, wxEXPAND);
            info->Add(grid, 0, wxEXPAND | wxTOP, 16);

            auto* badges = new wxBoxSizer(wxHORIZONTAL);
            badges->Add(Badge(body, "Confirmed", kLpStrip, kLpNavy), 0, wxALIGN_CENTER_VERTICAL);
            badges->Add(Badge(body, "Payment: Paid", kLpGreenBg, kLpGreen), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
            info->Add(badges, 0, wxTOP, 18);
            row->Add(info, 1, wxEXPAND | wxLEFT, 34);

            auto* bv = new wxBoxSizer(wxVERTICAL);
            bv->Add(row, 0, wxEXPAND | wxALL, 26);
            body->SetSizer(bv);
            ts->Add(body, 0, wxEXPAND | wxLEFT | wxRIGHT, 1);
        }

        // --- dashed separator
        ts->Add(new DashedLine(ticket, wxColour(205, 214, 230)), 0, wxEXPAND | wxLEFT | wxRIGHT, 1);

        // --- route info
        {
            auto* route = new wxPanel(ticket, wxID_ANY);
            route->SetBackgroundColour(*wxWHITE);
            auto* col = new wxBoxSizer(wxVERTICAL);
            auto* head = new wxBoxSizer(wxHORIZONTAL);
            head->Add(Label(route, "Route " + MidDot() + " " + v.routeName + " / " + v.routeCode, 8, kLpInk, true),
                      0, wxALIGN_CENTER_VERTICAL);
            head->AddStretchSpacer(1);
            head->Add(Label(route, v.seatNote + " " + MidDot() + " " + v.rideTime, 7, kLpMuted), 0,
                      wxALIGN_CENTER_VERTICAL);
            col->Add(head, 0, wxEXPAND);

            auto* tracker = new wxBoxSizer(wxHORIZONTAL);
            for (size_t i = 0; i < v.stops.size(); ++i) {
                const bool last = (i + 1 == v.stops.size());
                tracker->Add(StopDot(route, last ? kLpRed : kLpNavy, 8), 0, wxALIGN_CENTER_VERTICAL);
                tracker->Add(Label(route, v.stops[i], 7, kLpInk, last), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
                if (!last)
                    tracker->Add(Icon(route, kSvgChevronRight, kLpMuted, 12), 0,
                                 wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 12);
            }
            col->Add(tracker, 0, wxTOP, 14);

            auto* rv = new wxBoxSizer(wxVERTICAL);
            rv->Add(col, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 22);
            route->SetSizer(rv);
            ts->Add(route, 0, wxEXPAND | wxLEFT | wxRIGHT, 1);
        }

        // --- footer band
        {
            auto* band = new BandPanel(ticket, kLpSoft, 11, BandPanel::Round::Bottom);
            auto* row = new wxBoxSizer(wxHORIZONTAL);
            row->Add(Label(band, "Booking ID " + MidDot() + " " + v.bookingId, 6, kLpMuted), 0,
                     wxALIGN_CENTER_VERTICAL);
            row->AddStretchSpacer(1);
            row->Add(Label(band, "Valid for this trip only " + MidDot() + " " + v.fare + " paid", 6, kLpNavy, true),
                     0, wxALIGN_CENTER_VERTICAL);
            auto* bv = new wxBoxSizer(wxVERTICAL);
            bv->AddSpacer(14);
            bv->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 22);
            bv->AddSpacer(14);
            band->SetSizer(bv);
            ts->Add(band, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 1);
        }

        ticket->SetSizer(ts);
        split->Add(ticket, 5, wxEXPAND);
    }
    split->AddSpacer(18);

    // ===================== right: context cards
    {
        auto* col = new wxBoxSizer(wxVERTICAL);

        // --- "Your ride, live"
        {
            auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
            auto* inner = new wxBoxSizer(wxVERTICAL);

            auto* head = new wxBoxSizer(wxHORIZONTAL);
            head->Add(Label(card, "Your ride, live", 11, kLpInk, true), 0, wxALIGN_CENTER_VERTICAL);
            head->AddStretchSpacer(1);
            head->Add(Badge(card, "Boarding soon", kLpAmberBg, kLpAmber), 0, wxALIGN_CENTER_VERTICAL);
            inner->Add(head, 0, wxEXPAND);

            inner->Add(new MapPlaceholder(card), 0, wxEXPAND | wxTOP, 14);

            auto* where = new wxBoxSizer(wxHORIZONTAL);
            where->Add(Icon(card, kSvgPin, kLpNavy, 16), 0, wxALIGN_TOP | wxTOP, 2);
            auto* txt = new wxBoxSizer(wxVERTICAL);
            txt->Add(Label(card, v.bay, 8, kLpInk, true));
            txt->Add(Label(card, "Shuttle at origin " + MidDot() + " Updated " + v.updatedAt, 7, kLpMuted), 0, wxTOP, 4);
            where->Add(txt, 1, wxLEFT, 10);
            inner->Add(where, 0, wxEXPAND | wxTOP, 14);

            inner->Add(Link(card, "View current route  " + ArrowRight(), 7, kLpNavy, true,
                            [] { /* TODO: open live route */ }),
                       0, wxTOP, 14);

            auto* cv = new wxBoxSizer(wxVERTICAL);
            cv->Add(inner, 1, wxEXPAND | wxALL, 18);
            card->SetSizer(cv);
            col->Add(card, 0, wxEXPAND);
        }
        col->AddSpacer(18);

        // --- "Before you board"
        {
            auto* card = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
            auto* inner = new wxBoxSizer(wxVERTICAL);
            inner->Add(Label(card, "Before you board", 9, kLpInk, true));

            const struct { const char* svg; wxString text; } tips[] = {
                {kSvgIdCard, "Bring your student ID for verification."},
                {kSvgQr,     "Keep this pass ready for the driver."},
                {kSvgClock,  "Be at your bay by " + v.opensAt + "."},
            };
            auto* list = new wxFlexGridSizer(2, 12, 10);
            list->AddGrowableCol(1, 1);
            for (const auto& t : tips) {
                list->Add(Icon(card, t.svg, kLpMuted, 15), 0, wxALIGN_CENTER_VERTICAL);
                list->Add(Label(card, t.text, 7, kLpMuted), 0, wxALIGN_CENTER_VERTICAL);
            }
            inner->Add(list, 0, wxEXPAND | wxTOP, 16);

            auto* cv = new wxBoxSizer(wxVERTICAL);
            cv->Add(inner, 1, wxEXPAND | wxALL, 18);
            card->SetSizer(cv);
            col->Add(card, 0, wxEXPAND);
        }
        split->Add(col, 3, wxEXPAND);
    }
    pad->Add(split, 0, wxEXPAND);

    // ------------------------------------------------------------ bottom banner
    {
        pad->AddSpacer(18);
        auto* bar = new RoundedPanel(content, *wxWHITE, kLpBorder, 10);
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* text = new wxBoxSizer(wxVERTICAL);
        text->Add(Label(bar, "Plans changed? Manage your reservation.", 10, kLpInk, true));
        text->Add(Label(bar, "Review your trip details, payment receipt, and cancellation policy before departure.",
                        7, kLpMuted), 0, wxTOP, 5);
        row->Add(text, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(new PillButton(bar, "My reservations", *wxWHITE, kLpNavy, kLpBorder, 8, toReservations),
                 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);                          // connected
        auto* bv = new wxBoxSizer(wxVERTICAL);
        bv->AddSpacer(16);
        bv->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 18);
        bv->AddSpacer(16);
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