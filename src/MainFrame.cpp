#include "MainFrame.h"

#include "ReservationDialog.h"
#include "UiHelpers.h"

#include <wx/dateevt.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <memory>



MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "TaraMapuan | Campus Shuttle System",
              wxDefaultPosition, wxSize(1280, 800), wxDEFAULT_FRAME_STYLE) {
    LoadReservations();
    SetMinSize(wxSize(1050, 650));
    SetBackgroundColour(kCanvas);
    CreateStatusBar();
    SetStatusText("Prototype mode  |  All records and actions use local sample data");

    auto* root = new wxBoxSizer(wxHORIZONTAL);
    m_sidebar = new wxPanel(this, wxID_ANY);
    m_sidebar->SetBackgroundColour(kNavy);
    m_sidebar->SetMinSize(wxSize(226, -1));
    auto* sideSizer = new wxBoxSizer(wxVERTICAL);

    auto* brand = new wxStaticText(m_sidebar, wxID_ANY, "TaraMapuan");
    brand->SetFont(wxFontInfo(19).Bold());
    brand->SetForegroundColour(*wxWHITE);
    sideSizer->Add(brand, 0, wxLEFT | wxRIGHT | wxTOP, 18);
    auto* brandNote = new wxStaticText(m_sidebar, wxID_ANY, "CAMPUS MOBILITY");
    brandNote->SetFont(wxFontInfo(8).Bold());
    brandNote->SetForegroundColour(wxColour(184, 203, 230));
    sideSizer->Add(brandNote, 0, wxLEFT | wxRIGHT | wxBOTTOM, 18);

    m_navScroll = new wxScrolledWindow(m_sidebar, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxVSCROLL | wxBORDER_NONE);
    m_navScroll->SetScrollRate(0, 8);
    m_navScroll->SetBackgroundColour(kNavy);
    m_navSizer = new wxBoxSizer(wxVERTICAL);
    m_navScroll->SetSizer(m_navSizer);
    sideSizer->Add(m_navScroll, 1, wxEXPAND);
    auto* prototypeLabel = new wxStaticText(m_sidebar, wxID_ANY, "DEMO DATA  |  LOCAL PREVIEW");
    prototypeLabel->SetForegroundColour(wxColour(184, 203, 230));
    prototypeLabel->SetFont(wxFontInfo(8).Bold());
    sideSizer->Add(prototypeLabel, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 16);
    m_sidebar->SetSizer(sideSizer);
    root->Add(m_sidebar, 0, wxEXPAND);

    auto* main = new wxPanel(this, wxID_ANY);
    main->SetBackgroundColour(kCanvas);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* topbar = new wxPanel(main, wxID_ANY);
    topbar->SetBackgroundColour(*wxWHITE);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* titleBox = new wxBoxSizer(wxVERTICAL);
    m_pageTitle = new wxStaticText(topbar, wxID_ANY, "Dashboard");
    m_pageTitle->SetFont(wxFontInfo(16).Bold());
    m_pageTitle->SetForegroundColour(kNavy);
    m_pageSubtitle = new wxStaticText(topbar, wxID_ANY, "Campus transportation overview");
    m_pageSubtitle->SetForegroundColour(kMuted);
    titleBox->Add(m_pageTitle);
    titleBox->Add(m_pageSubtitle, 0, wxTOP, 2);
    topSizer->Add(titleBox, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 16);
    auto* sampleTag = new wxStaticText(topbar, wxID_ANY, "DEMO  |  LOCAL PREVIEW");
    sampleTag->SetForegroundColour(kBlue);
    sampleTag->SetFont(wxFontInfo(8).Bold());
    topSizer->Add(sampleTag, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    topbar->SetSizer(topSizer);
    mainSizer->Add(topbar, 0, wxEXPAND);

    m_pageHost = new wxPanel(main, wxID_ANY);
    m_pageHost->SetBackgroundColour(kCanvas);
    m_pageSizer = new wxBoxSizer(wxVERTICAL);
    m_pageHost->SetSizer(m_pageSizer);
    mainSizer->Add(m_pageHost, 1, wxEXPAND | wxALL, 14);
    main->SetSizer(mainSizer);
    root->Add(main, 1, wxEXPAND);
    SetSizer(root);
    ShowPage(Page::Landing);
    Centre();
}

void MainFrame::LoadReservations() {
    // TODO: Replace the local INI demonstration store with a database-backed repository and migrations.
    std::unique_ptr<wxFileConfig> config(ReservationConfig());
    long count = 0;
    if (!config->Read("Reservations/Count", &count) || count < 0 || count > 100000) return;
    m_reservations.clear();
    for (long i = 0; i < count; ++i) {
        const wxString path = wxString::Format("Reservations/Record%ld/", i);
        ReservationRecord r;
        auto readString = [&](const wxString& key, std::string& value) {
            wxString stored;
            config->Read(path + key, &stored);
            value = Utf8(stored);
        };
        readString("Id", r.id);
        readString("Passenger", r.passenger);
        readString("StudentId", r.studentId);
        readString("Route", r.route);
        readString("Departure", r.departure);
        readString("Seat", r.seat);
        readString("PaymentMethod", r.paymentMethod);
        readString("PaymentStatus", r.paymentStatus);
        readString("Status", r.status);
        readString("PaymentReference", r.paymentReference);
        readString("BoardingStatus", r.boardingStatus);
        config->Read(path + "SeatCount", &r.seatCount, 1);
        config->Read(path + "ScheduleIndex", &r.scheduleIndex, -1);
        m_reservations.push_back(std::move(r));
    }
    config->Read("Reservations/Sequence", &m_reservationSequence, 24100);
    for (auto& schedule : m_schedules) schedule.availableSeats = schedule.capacity;
    for (const auto& r : m_reservations) {
        if (!IsActiveReservation(r) || r.scheduleIndex < 0 || r.scheduleIndex >= static_cast<int>(m_schedules.size())) continue;
        auto& schedule = m_schedules[static_cast<size_t>(r.scheduleIndex)];
        schedule.availableSeats = std::max(0, schedule.availableSeats - r.seatCount);
    }
    for (auto& schedule : m_schedules) {
        if (schedule.availableSeats == 0) schedule.status = "Full";
        else if (schedule.status == "Full") schedule.status = "Available";
    }
}

void MainFrame::SaveReservations() const {
    std::unique_ptr<wxFileConfig> config(ReservationConfig());
    config->DeleteGroup("Reservations");
    config->Write("Reservations/Count", static_cast<long>(m_reservations.size()));
    config->Write("Reservations/Sequence", m_reservationSequence);
    for (size_t i = 0; i < m_reservations.size(); ++i) {
        const auto& r = m_reservations[i];
        const wxString path = wxString::Format("Reservations/Record%zu/", i);
        config->Write(path + "Id", W(r.id));
        config->Write(path + "Passenger", W(r.passenger));
        config->Write(path + "StudentId", W(r.studentId));
        config->Write(path + "Route", W(r.route));
        config->Write(path + "Departure", W(r.departure));
        config->Write(path + "Seat", W(r.seat));
        config->Write(path + "PaymentMethod", W(r.paymentMethod));
        config->Write(path + "PaymentStatus", W(r.paymentStatus));
        config->Write(path + "Status", W(r.status));
        config->Write(path + "PaymentReference", W(r.paymentReference));
        config->Write(path + "BoardingStatus", W(r.boardingStatus));
        config->Write(path + "SeatCount", r.seatCount);
        config->Write(path + "ScheduleIndex", r.scheduleIndex);
    }
    config->Flush();
}

wxScrolledWindow* MainFrame::StartPage(const wxString& title, const wxString& subtitle) {
    auto* page = new wxScrolledWindow(m_pageHost, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxVSCROLL | wxBORDER_NONE);
    page->SetScrollRate(0, 10);
    page->SetBackgroundColour(kCanvas);
    auto* layout = new wxBoxSizer(wxVERTICAL);
    auto* heading = new wxStaticText(page, wxID_ANY, title);
    heading->SetFont(wxFontInfo(20).Bold());
    heading->SetForegroundColour(kNavy);
    layout->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, 4);
    auto* desc = new wxStaticText(page, wxID_ANY, subtitle);
    desc->SetForegroundColour(kMuted);
    desc->Wrap(900);
    layout->Add(desc, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);
    page->SetSizer(layout);
    return page;
}

void MainFrame::ShowPage(Page page) {
    m_currentPage = page;
    const std::vector<std::pair<Page, wxString>> titles = {
        {Page::Landing,"TaraMapuan Home"},{Page::RoleSelect,"Choose your portal"},
        {Page::Login,"Demo login"},{Page::Dashboard,"Dashboard"},
        {Page::StudentDashboard,"Student Dashboard"},{Page::Reserve,"Reserve a Shuttle"},
        {Page::Reservations,"My Reservations"},{Page::Routes,"Shuttle Routes"},
        {Page::Schedules,"Shuttle Schedules"},{Page::Passengers,"Passenger Management"},
        {Page::Drivers,"Driver Management"},{Page::DriverDashboard,"Driver Dashboard"},
        {Page::AssignedTrips,"Assigned Trips"},{Page::DigitalPass,"My Digital Pass"},
        {Page::Manifest,"Passenger Manifest"},{Page::Monitoring,"Shuttle Monitoring"},
        {Page::Boarding,"Boarding Check-in"},{Page::TripHistory,"Trip History"},
        {Page::Payments,"Payment Verification"},{Page::Reports,"Reports"},{Page::Settings,"Settings"}
    };
    wxString title = "Dashboard";
    for (const auto& item : titles) if (item.first == page) { title = item.second; break; }
    m_pageTitle->SetLabel(title);
    m_pageSubtitle->SetLabel(m_userRole == UserRole::None
        ? "Campus shuttle reservations and trip operations"
        : (m_userRole == UserRole::Student ? "Student portal  |  Local demo data" : "Driver portal  |  Local demo data"));
    m_sidebar->Show(m_userRole != UserRole::None);
    RebuildNavigation();

    m_pageSizer->Clear(false);
    const wxWindowList& children = m_pageHost->GetChildren();
    for (wxWindowList::compatibility_iterator node = children.GetFirst(); node; node = node->GetNext())
        node->GetData()->Destroy();
    m_pageSizer->Add(BuildPage(page), 1, wxEXPAND);
    UpdateNavigationStyle();
    m_pageHost->Layout();
    Layout();
}

void MainFrame::RefreshCurrentPage() { ShowPage(m_currentPage); }

void MainFrame::UpdateNavigationStyle() {
    for (const auto& item : m_navigation) {
        const bool selected = item.first == m_currentPage;
        item.second->SetBackgroundColour(selected ? kPrimaryRed : kNavy);
        item.second->SetForegroundColour(*wxWHITE);
        item.second->Refresh();
    }
}

void MainFrame::RebuildNavigation() {
    if (!m_navSizer) return;
    m_navSizer->Clear(true);
    m_navigation.clear();
    std::vector<std::pair<Page, wxString>> pages;
    if (m_userRole == UserRole::Student) {
        pages = {{Page::StudentDashboard,"Dashboard"},{Page::Schedules,"Shuttle Schedules"},
                 {Page::Reserve,"Reserve a Seat"},{Page::Reservations,"My Reservations"},
                 {Page::DigitalPass,"My Digital Pass"},{Page::Payments,"Payment Details"},
                 {Page::Settings,"Settings"},{Page::Landing,"Logout"}};
    } else if (m_userRole == UserRole::Driver) {
        pages = {{Page::DriverDashboard,"Dashboard"},{Page::AssignedTrips,"Assigned Trips"},
                 {Page::Manifest,"Passenger Manifest"},{Page::Payments,"Payment Verification"},
                 {Page::Boarding,"Boarding Check-in"},{Page::TripHistory,"Trip History"},
                 {Page::Schedules,"Shuttle Schedules"},{Page::Routes,"Shuttle Routes"},
                 {Page::Monitoring,"Shuttle Monitoring"},{Page::Passengers,"Passenger Directory"},
                 {Page::Drivers,"Driver Directory"},{Page::Reports,"Reports"},
                 {Page::Settings,"Settings"},{Page::Landing,"Logout"}};
    }
    for (const auto& entry : pages) {
        auto* button = new wxButton(m_navScroll, wxID_ANY, entry.second, wxDefaultPosition,
                                    wxDefaultSize, wxBORDER_NONE);
        button->SetMinSize(wxSize(-1, 38));
        button->SetBackgroundColour(kNavy);
        button->SetForegroundColour(wxColour(232, 238, 247));
        button->SetFont(wxFontInfo(9));
        m_navSizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
        m_navigation.emplace_back(entry.first, button);
        button->Bind(wxEVT_BUTTON, [this, destination = entry.first](wxCommandEvent&) {
            if (destination == Page::Landing && m_userRole != UserRole::None) {
                m_userRole = UserRole::None;
                m_selectedRole = UserRole::None;
                m_currentUserId.clear();
                m_selectedSchedule = -1;
                m_selectedReservationId.clear();
                ShowPage(Page::Landing);
                return;
            }
            ShowPage(destination);
        });
    }
    m_navScroll->FitInside();
    m_navScroll->Layout();
}

