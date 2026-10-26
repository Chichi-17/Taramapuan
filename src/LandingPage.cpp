#include "MainFrame.h"
#include "UiHelpers.h"

#include <wx/filename.h>
#include <wx/image.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>

wxScrolledWindow* MainFrame::BuildLandingPage() {
    auto* page = new wxScrolledWindow(m_pageHost, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxVSCROLL | wxBORDER_NONE);
    page->SetScrollRate(0, 10);
    page->SetBackgroundColour(*wxWHITE);
    auto* layout = new wxBoxSizer(wxVERTICAL);

    // Public site navigation.
    auto* nav = new wxPanel(page, wxID_ANY);
    nav->SetBackgroundColour(*wxWHITE);
    auto* navLayout = new wxBoxSizer(wxHORIZONTAL);
    auto* mark = new wxStaticText(nav, wxID_ANY, "▣");
    mark->SetForegroundColour(kPrimaryRed);
    mark->SetFont(wxFontInfo(15).Bold());
    mark->SetLabel("T");
    mark->SetBackgroundColour(kPrimaryRed);
    mark->SetForegroundColour(*wxWHITE);
    auto* brand = new wxStaticText(nav, wxID_ANY, "TaraMapuan");
    brand->SetForegroundColour(kNavy);
    brand->SetFont(wxFontInfo(13).Bold());
    wxFileName logoPath(wxStandardPaths::Get().GetExecutablePath());
    logoPath.SetFullName(wxEmptyString);
    logoPath.AppendDir("assets");
    logoPath.SetFullName("TARA MAPUAN-LOGO.png");
    bool logoAdded = false;
    if (wxFileExists(logoPath.GetFullPath())) {
        wxImage logo(logoPath.GetFullPath(), wxBITMAP_TYPE_PNG);
        if (logo.IsOk()) {
            logo.Rescale(46, 46, wxIMAGE_QUALITY_HIGH);
            auto* logoImage = new wxStaticBitmap(nav, wxID_ANY, wxBitmap(logo));
            logoImage->SetMinSize(wxSize(46, 46));
            navLayout->Add(logoImage, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
            logoAdded = true;
        }
    }
    if (!logoAdded) navLayout->Add(mark, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    navLayout->Add(brand, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);

    auto* login = MakeButton(nav, "Login");
    auto* reserve = MakeButton(nav, "Reserve Seat", true);
    reserve->SetBackgroundColour(kPrimaryRed);
    reserve->SetForegroundColour(*wxWHITE);
    login->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    reserve->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    login->SetMinSize(wxSize(-1, 32));
    reserve->SetMinSize(wxSize(-1, 32));
    navLayout->Add(login, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);
    navLayout->Add(reserve, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    nav->SetSizer(navLayout);
    layout->Add(nav, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    // Departure strip gives the public home page a live-looking schedule summary.
    auto* strip = new wxPanel(page, wxID_ANY);
    strip->SetBackgroundColour(wxColour(236, 243, 251));
    auto* stripLayout = new wxBoxSizer(wxHORIZONTAL);
    auto* stripIntro = new wxBoxSizer(wxVERTICAL);
    auto* move = new wxStaticText(strip, wxID_ANY, "SHUTTLE DEPARTURES");
    move->SetForegroundColour(kNavy);
    move->SetFont(wxFontInfo(9).Bold());
    auto* updated = new wxStaticText(strip, wxID_ANY, "Sample times for today");
    updated->SetForegroundColour(kMuted);
    updated->SetFont(wxFontInfo(8));
    stripIntro->Add(move);
    stripIntro->Add(updated, 0, wxTOP, 2);
    stripLayout->Add(stripIntro, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);

    const size_t visibleTrips = std::min<size_t>(4, m_schedules.size());
    for (size_t i = 0; i < visibleTrips; ++i) {
        const auto& trip = m_schedules[i];
        const auto route = std::find_if(m_routes.begin(), m_routes.end(), [&](const RouteRecord& item) {
            return item.id == trip.routeId;
        });
        const wxString routeName = route == m_routes.end() ? W(trip.routeId) : W(route->name);
        auto* item = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(strip, wxID_ANY, routeName);
        name->SetForegroundColour(kNavy);
        name->SetFont(wxFontInfo(8).Bold());
        auto* meta = new wxStaticText(strip, wxID_ANY, W(trip.shuttle + " · " + trip.driver));
        meta->SetForegroundColour(kMuted);
        meta->SetFont(wxFontInfo(8));
        auto* time = new wxStaticText(strip, wxID_ANY, W(trip.departure));
        time->SetForegroundColour(kPrimaryRed);
        time->SetFont(wxFontInfo(9).Bold());
        auto* status = new wxStaticText(strip, wxID_ANY,
            trip.availableSeats > 0 ? wxString::Format("%d seats left", trip.availableSeats) : wxString("Boarding soon"));
        status->SetForegroundColour(kGreen);
        status->SetFont(wxFontInfo(8));
        item->Add(name);
        item->Add(meta, 0, wxTOP, 2);
        item->Add(time, 0, wxTOP, 2);
        item->Add(status, 0, wxTOP, 1);
        stripLayout->Add(item, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    }
    strip->SetSizer(stripLayout);
    layout->Add(strip, 0, wxEXPAND | wxTOP, 8);

    // Use the full hero width for the message and actions, without a photo panel.
    auto* hero = new wxPanel(page, wxID_ANY);
    hero->SetBackgroundColour(kNavy);
    auto* heroLayout = new wxBoxSizer(wxHORIZONTAL);
    auto* heroCopy = new wxPanel(hero, wxID_ANY);
    heroCopy->SetBackgroundColour(kNavy);
    auto* copyLayout = new wxBoxSizer(wxVERTICAL);
    auto* eyebrow = new wxStaticText(heroCopy, wxID_ANY, "CAMPUS SHUTTLE SERVICE");
    eyebrow->SetFont(wxFontInfo(8).Bold());
    eyebrow->SetForegroundColour(wxColour(202, 215, 232));
    copyLayout->Add(eyebrow, 0, wxBOTTOM, 12);
    auto* headline = new wxStaticText(heroCopy, wxID_ANY, "Your Route.\nYour Shuttle.\nYour Ride.");
    headline->SetFont(wxFontInfo(27).Bold());
    headline->SetForegroundColour(*wxWHITE);
    copyLayout->Add(headline, 0, wxBOTTOM, 12);
    auto* message = new wxStaticText(heroCopy, wxID_ANY,
        "From your first class to your last stop. Find your route, choose a shuttle, and save your seat with TaraMapuan.");
    message->SetFont(wxFontInfo(9));
    message->SetForegroundColour(wxColour(220, 229, 241));
    message->Wrap(std::max(430, m_pageHost->GetClientSize().GetWidth() - 110));
    copyLayout->Add(message, 0, wxBOTTOM, 14);
    auto* heroActions = new wxBoxSizer(wxHORIZONTAL);
    auto* findRoute = MakeButton(heroCopy, "Find my route", true);
    auto* viewSchedule = MakeButton(heroCopy, "View shuttle schedule");
    findRoute->SetBackgroundColour(kPrimaryRed);
    findRoute->SetForegroundColour(*wxWHITE);
    viewSchedule->SetBackgroundColour(wxColour(238, 243, 250));
    viewSchedule->SetForegroundColour(kNavy);
    findRoute->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    viewSchedule->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    heroActions->Add(findRoute, 0, wxRIGHT, 10);
    heroActions->Add(viewSchedule, 0);
    copyLayout->Add(heroActions, 0);
    heroCopy->SetSizer(copyLayout);
    heroLayout->Add(heroCopy, 1, wxEXPAND | wxALL, 26);

    hero->SetSizer(heroLayout);
    layout->Add(hero, 0, wxEXPAND);

    // The pickup stops and nearby destinations are listed separately because
    // no specific stop-to-destination pairings were provided.
    auto* lower = new wxPanel(page, wxID_ANY);
    lower->SetBackgroundColour(*wxWHITE);
    auto* lowerLayout = new wxBoxSizer(wxHORIZONTAL);
    const auto addLocationGroup = [lower](const wxString& heading,
                                           const wxString& note,
                                           const std::vector<wxString>& locations,
                                           const wxColour& accent) {
        auto* group = new wxPanel(lower, wxID_ANY);
        group->SetBackgroundColour(*wxWHITE);
        auto* groupLayout = new wxBoxSizer(wxVERTICAL);
        auto* title = new wxStaticText(group, wxID_ANY, heading);
        title->SetForegroundColour(kNavy);
        title->SetFont(wxFontInfo(13).Bold());
        groupLayout->Add(title, 0, wxBOTTOM, 4);
        auto* caption = new wxStaticText(group, wxID_ANY, note);
        caption->SetForegroundColour(kMuted);
        caption->SetFont(wxFontInfo(9));
        groupLayout->Add(caption, 0, wxBOTTOM, 10);
        auto* locationGrid = new wxGridSizer(2, 7, 7);
        for (size_t i = 0; i < locations.size(); ++i) {
            auto* card = new wxPanel(group, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
            card->SetBackgroundColour(wxColour(239, 245, 252));
            auto* cardLayout = new wxBoxSizer(wxVERTICAL);
            auto* marker = new wxStaticText(card, wxID_ANY,
                wxString::Format("%02u", static_cast<unsigned>(i + 1)));
            marker->SetForegroundColour(accent);
            marker->SetFont(wxFontInfo(9).Bold());
            auto* name = new wxStaticText(card, wxID_ANY, locations[i]);
            name->SetForegroundColour(kNavy);
            name->SetFont(wxFontInfo(10).Bold());
            cardLayout->Add(marker, 0, wxBOTTOM, 4);
            cardLayout->Add(name, 0, wxBOTTOM, 8);
            card->SetSizer(cardLayout);
            locationGrid->Add(card, 1, wxEXPAND);
        }
        groupLayout->Add(locationGrid, 0, wxEXPAND);
        group->SetSizer(groupLayout);
        return group;
    };
    auto* schoolGroup = addLocationGroup(
        "School pickup areas", "Pickup points around campus",
        {"7/11", "Airwell", "Einstein Bldg", "Shuttle Area"}, kPrimaryRed);
    auto* cityGroup = addLocationGroup(
        "Non-school areas", "Nearby towns and destinations",
        {"Calamba", "San Pedro", "Paseo", "Sta Rosa"}, kBlue);
    lowerLayout->Add(schoolGroup, 1, wxEXPAND | wxALL, 18);
    lowerLayout->Add(cityGroup, 1, wxEXPAND | wxALL, 18);
    lower->SetSizer(lowerLayout);
    layout->Add(lower, 0, wxEXPAND);

    auto* footer = new wxPanel(page, wxID_ANY);
    footer->SetBackgroundColour(wxColour(244, 247, 251));
    auto* footerLayout = new wxBoxSizer(wxHORIZONTAL);
    auto* footerCopy = new wxBoxSizer(wxVERTICAL);
    auto* footerTitle = new wxStaticText(footer, wxID_ANY, "Built for the Mapuan on the move.");
    footerTitle->SetForegroundColour(kNavy);
    footerTitle->SetFont(wxFontInfo(8).Bold());
    auto* footerNote = new wxStaticText(footer, wxID_ANY, "A connected, reliable campus shuttle experience.  © 2026 TaraMapuan");
    footerNote->SetForegroundColour(kMuted);
    footerNote->SetFont(wxFontInfo(8));
    footerCopy->Add(footerTitle);
    footerCopy->Add(footerNote, 0, wxTOP, 3);
    footerLayout->Add(footerCopy, 1, wxALIGN_CENTER_VERTICAL | wxALL, 16);
    auto* help = new wxStaticText(footer, wxID_ANY, "Transport help desk     Privacy policy     Driver login  ↗");
    help->SetForegroundColour(kBlue);
    help->SetFont(wxFontInfo(8));
    footerLayout->Add(help, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    footer->SetSizer(footerLayout);
    layout->Add(footer, 0, wxEXPAND);

    findRoute->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::RoleSelect); });
    viewSchedule->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::Schedules); });
    login->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::RoleSelect); });
    reserve->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::RoleSelect); });

    page->SetSizer(layout);
    return page;
}
