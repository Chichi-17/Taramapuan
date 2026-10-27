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
              wxDefaultPosition, wxSize(1280, 800), wxDEFAULT_FRAME_STYLE),
      m_waitlistTimer(this) {
    LoadRoutesAndSchedules();
    LoadReservations();
    SetMinSize(wxSize(760, 560));
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
    m_topbar = new wxPanel(main, wxID_ANY);
    m_topbar->SetBackgroundColour(*wxWHITE);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* titleBox = new wxBoxSizer(wxVERTICAL);
    m_pageTitle = new wxStaticText(m_topbar, wxID_ANY, "Dashboard");
    m_pageTitle->SetFont(wxFontInfo(16).Bold());
    m_pageTitle->SetForegroundColour(kNavy);
    m_pageSubtitle = new wxStaticText(m_topbar, wxID_ANY, "Campus transportation overview");
    m_pageSubtitle->SetForegroundColour(kMuted);
    titleBox->Add(m_pageTitle);
    titleBox->Add(m_pageSubtitle, 0, wxTOP, 2);
    topSizer->Add(titleBox, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 16);
    auto* sampleTag = new wxStaticText(m_topbar, wxID_ANY, "DEMO  |  LOCAL PREVIEW");
    sampleTag->SetForegroundColour(kBlue);
    sampleTag->SetFont(wxFontInfo(8).Bold());
    topSizer->Add(sampleTag, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    m_topbar->SetSizer(topSizer);
    mainSizer->Add(m_topbar, 0, wxEXPAND);

    m_pageHost = new wxPanel(main, wxID_ANY);
    m_pageHost->SetBackgroundColour(kCanvas);
    m_pageSizer = new wxBoxSizer(wxVERTICAL);
    m_pageHost->SetSizer(m_pageSizer);
    mainSizer->Add(m_pageHost, 1, wxEXPAND | wxALL, 14);
    main->SetSizer(mainSizer);
    root->Add(main, 1, wxEXPAND);
    SetSizer(root);
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { ProcessWaitlistCutoffs(); }, m_waitlistTimer.GetId());
    m_waitlistTimer.Start(15000);
    ProcessWaitlistCutoffs();
    ShowPage(Page::Landing);
    Centre();
}

void MainFrame::LoadRoutesAndSchedules() {
    std::unique_ptr<wxFileConfig> config(ReservationConfig());
    long count = 0;
    if (config->Read("Routes/Count", &count) && count >= 0 && count <= 10000) {
        m_routes.clear();
        for (long i = 0; i < count; ++i) {
            const wxString path = wxString::Format("Routes/Record%ld/", i);
            RouteRecord route;
            auto readString = [&](const wxString& key, std::string& value) {
                wxString stored;
                config->Read(path + key, &stored);
                value = Utf8(stored);
            };
            readString("Id", route.id);
            readString("Name", route.name);
            readString("Pickup", route.pickup);
            readString("Destination", route.destination);
            readString("Shuttle", route.shuttle);
            readString("TravelTime", route.travelTime);
            readString("Status", route.status);
            m_routes.push_back(std::move(route));
        }
    }

    count = 0;
    if (config->Read("Schedules/Count", &count) && count >= 0 && count <= 10000) {
        m_schedules.clear();
        for (long i = 0; i < count; ++i) {
            const wxString path = wxString::Format("Schedules/Record%ld/", i);
            ScheduleRecord schedule;
            auto readString = [&](const wxString& key, std::string& value) {
                wxString stored;
                config->Read(path + key, &stored);
                value = Utf8(stored);
            };
            readString("Id", schedule.id);
            readString("RouteId", schedule.routeId);
            readString("Departure", schedule.departure);
            readString("Arrival", schedule.arrival);
            readString("Driver", schedule.driver);
            readString("Shuttle", schedule.shuttle);
            readString("Status", schedule.status);
            readString("DriverId", schedule.driverId);
            config->Read(path + "AvailableSeats", &schedule.availableSeats, 0);
            config->Read(path + "Capacity", &schedule.capacity, 0);
            m_schedules.push_back(std::move(schedule));
        }
    }
}

void MainFrame::SaveRoutesAndSchedules() const {
    {
        std::unique_ptr<wxFileConfig> config(ReservationConfig());
        config->DeleteGroup("Routes");
        config->Write("Routes/Count", static_cast<long>(m_routes.size()));
        for (size_t i = 0; i < m_routes.size(); ++i) {
            const auto& route = m_routes[i];
            const wxString path = wxString::Format("Routes/Record%zu/", i);
            config->Write(path + "Id", W(route.id));
            config->Write(path + "Name", W(route.name));
            config->Write(path + "Pickup", W(route.pickup));
            config->Write(path + "Destination", W(route.destination));
            config->Write(path + "Shuttle", W(route.shuttle));
            config->Write(path + "TravelTime", W(route.travelTime));
            config->Write(path + "Status", W(route.status));
        }
        config->DeleteGroup("Schedules");
        config->Write("Schedules/Count", static_cast<long>(m_schedules.size()));
        for (size_t i = 0; i < m_schedules.size(); ++i) {
            const auto& schedule = m_schedules[i];
            const wxString path = wxString::Format("Schedules/Record%zu/", i);
            config->Write(path + "Id", W(schedule.id));
            config->Write(path + "RouteId", W(schedule.routeId));
            config->Write(path + "Departure", W(schedule.departure));
            config->Write(path + "Arrival", W(schedule.arrival));
            config->Write(path + "Driver", W(schedule.driver));
            config->Write(path + "Shuttle", W(schedule.shuttle));
            config->Write(path + "Status", W(schedule.status));
            config->Write(path + "DriverId", W(schedule.driverId));
            config->Write(path + "AvailableSeats", schedule.availableSeats);
            config->Write(path + "Capacity", schedule.capacity);
        }
        config->Flush();
    }
    SaveReservations();
}

void MainFrame::LoadReservations() {
    // TODO: Replace the local INI demonstration store with a database-backed repository and migrations.
    m_reservationQueue.clear();
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
        config->Read(path + "WaitlistRequested", &r.waitlistRequested, false);
        m_reservations.push_back(std::move(r));
    }
    for (const auto& reservation : m_reservations) {
        if (reservation.status == "Waitlisted" && reservation.scheduleIndex >= 0 &&
            reservation.scheduleIndex < static_cast<int>(m_schedules.size())) {
            m_reservationQueue.enqueue(reservation);
        }
    }
    config->Read("Reservations/Sequence", &m_reservationSequence, 24100);
    for (auto& schedule : m_schedules) schedule.availableSeats = schedule.capacity;
    for (const auto& r : m_reservations) {
        if (!HasAllocatedSeats(r) || r.scheduleIndex < 0 || r.scheduleIndex >= static_cast<int>(m_schedules.size())) continue;
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
        config->Write(path + "WaitlistRequested", r.waitlistRequested);
    }
    config->Flush();
}

void MainFrame::ProcessWaitlistCutoffs() {
    bool changed = false;
    for (auto& reservation : m_reservations) {
        if (reservation.scheduleIndex < 0 ||
            reservation.scheduleIndex >= static_cast<int>(m_schedules.size()) ||
            reservation.status == "Cancelled" ||
            reservation.status == "Waitlist expired" ||
            reservation.status == "No-show (walk-in release)" ||
            reservation.status == "Request expired" ||
            reservation.boardingStatus == "Boarded" ||
            !IsPastDepartureCutoff(W(reservation.departure), 10)) {
            continue;
        }

        if (reservation.status == "Waitlisted") {
            reservation.status = "Waitlist expired";
            reservation.paymentStatus = "Not due - waitlist closed";
            reservation.boardingStatus = "Waitlist expired";
            while (!m_reservationQueue.isEmpty(reservation.scheduleIndex, reservation.departure)) {
                m_reservationQueue.dequeue(reservation.scheduleIndex, reservation.departure);
            }
        } else if (reservation.status == "Pending driver approval") {
            reservation.status = "Request expired";
            reservation.paymentStatus = "Not due - request expired";
            reservation.boardingStatus = "Request expired";
        } else if (HasAllocatedSeats(reservation)) {
            auto& schedule = m_schedules[static_cast<size_t>(reservation.scheduleIndex)];
            schedule.availableSeats = std::min(schedule.capacity,
                schedule.availableSeats + reservation.seatCount);
            reservation.status = "No-show (walk-in release)";
            reservation.seat = "--";
            reservation.boardingStatus = "No-show";
        } else {
            continue;
        }
        changed = true;
    }

    for (auto& schedule : m_schedules) {
        if (schedule.availableSeats > 0 && schedule.status == "Full") {
            schedule.status = "Available";
        }
    }

    if (changed) {
        SaveReservations();
        SetStatusText("10-minute check-in cutoff passed; unclaimed seats are open for on-site walk-ins.");
        RefreshCurrentPage();
    }
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
    const unsigned long revision = ++m_pageRevision;
    CallAfter([this, page, revision] {
        if (revision == m_pageRevision) RenderPage(page);
    });
}

void MainFrame::RenderPage(Page page) {
    const std::vector<std::pair<Page, wxString>> titles = {
        {Page::Landing,"TaraMapuan Home"},{Page::RoleSelect,"Choose your portal"},
        {Page::Login,"Demo login"},{Page::Dashboard,"Dashboard"},
        {Page::StudentDashboard,"Student Dashboard"},{Page::Profile,"My Profile"},{Page::Reserve,"Reserve a Shuttle"},
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
    m_topbar->Show(page != Page::Landing);
    if (GetStatusBar()) GetStatusBar()->Show(page != Page::Landing);
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
        pages = {{Page::StudentDashboard,"Dashboard"},{Page::Profile,"My Profile"},{Page::Schedules,"Shuttle Schedules"},
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
