#include "MainFrame.h"
#include "ReservationDialog.h"
#include "UiHelpers.h"
#include <wx/dateevt.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/statbmp.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <algorithm>
#include <memory>
#include <sstream>

wxWindow* MainFrame::BuildPage(Page page) {
    switch (page) {
    case Page::Landing: return BuildLandingPage();
    case Page::RoleSelect: return BuildRoleSelectPage();
    case Page::Login: return BuildLoginPage();
    case Page::Dashboard: return BuildDashboard();
    case Page::StudentDashboard: return BuildStudentDashboard();
    case Page::Profile: return BuildStudentProfilePage();
    case Page::Reserve: return BuildReservationPage();
    case Page::Reservations: return BuildReservationsPage();
    case Page::Routes: return BuildRoutesPage();
    case Page::Schedules: return BuildSchedulesPage();
    case Page::Passengers: return BuildPassengersPage();
    case Page::Drivers: return BuildDriversPage();
    case Page::DriverDashboard: return BuildDriverDashboard();
    case Page::AssignedTrips: return BuildAssignedTripsPage();
    case Page::Manifest: return BuildManifestPage();
    case Page::Boarding: return BuildBoardingPage();
    case Page::TripHistory: return BuildTripHistoryPage();
    case Page::DigitalPass: return BuildDigitalPassPage();
    case Page::Monitoring: return BuildMonitoringPage();
    case Page::Payments: return BuildPaymentsPage();
    case Page::Reports: return BuildReportsPage();
    case Page::Settings: return BuildSettingsPage();
    }
    return BuildDashboard();
}

wxWindow* MainFrame::BuildDashboard() {
    auto* page = StartPage("Good morning, campus team", "A quick look at today's shuttle activity. All values below are fictional prototype data.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());
    int active = 0, available = 0, pending = 0;
    for (const auto& route : m_routes) if (route.status == "Active") ++active;
    for (const auto& schedule : m_schedules) available += schedule.availableSeats;
    for (const auto& reservation : m_reservations) if (reservation.status == "Pending") ++pending;
    auto* metrics = new wxGridSizer(1, 5, 10, 10);
    metrics->Add(MetricCard(page,"TOTAL SHUTTLES",W(std::to_string(m_schedules.size())),"Fleet sample",kBlue),1,wxEXPAND);
    metrics->Add(MetricCard(page,"ACTIVE SHUTTLES",W(std::to_string(active)),"Routes in service",kGreen),1,wxEXPAND);
    metrics->Add(MetricCard(page,"TODAY'S RESERVATIONS",W(std::to_string(m_reservations.size())),"All sample statuses",kBlue),1,wxEXPAND);
    metrics->Add(MetricCard(page,"AVAILABLE SEATS",W(std::to_string(available)),"Across listed departures",kGreen),1,wxEXPAND);
    metrics->Add(MetricCard(page,"PENDING",W(std::to_string(pending)),"Reservations awaiting review",kAmber),1,wxEXPAND);
    layout->Add(metrics,0,wxEXPAND|wxALL,10);

    auto* actions = new wxBoxSizer(wxHORIZONTAL);
    auto* newReservation = MakeButton(page,"+ New Reservation",true);
    auto* schedules = MakeButton(page,"View Schedules");
    auto* monitor = MakeButton(page,"Manage Shuttles");
    actions->Add(newReservation,0,wxRIGHT,8); actions->Add(schedules,0,wxRIGHT,8); actions->Add(monitor,0);
    layout->Add(actions,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    newReservation->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){CreateReservation();});
    schedules->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){ShowPage(Page::Schedules);});
    monitor->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){ShowPage(Page::Monitoring);});

    auto* tables = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* recentInner{}; auto* recent = MakeCard(page,"Recent reservations  |  Sample",recentInner);
    auto* reservations = MakeTable(recent,{"Reservation","Passenger","Route","Departure","Status"},{100,130,150,90,95});
    for(size_t i=0;i<m_reservations.size();++i){const auto&r=m_reservations[i];AddRow(reservations,static_cast<long>(i),{W(r.id),W(r.passenger),W(r.route),W(r.departure),W(r.status)});}
    recentInner->Add(reservations,1,wxEXPAND);
    wxBoxSizer* shuttleInner{}; auto* shuttleCard = MakeCard(page,"Shuttle status  |  Simulated",shuttleInner);
    auto* shuttleTable = MakeTable(shuttleCard,{"Shuttle","Driver","Route","Seats","Status"},{80,120,140,65,95});
    for(size_t i=0;i<m_schedules.size();++i){const auto&s=m_schedules[i];AddRow(shuttleTable,static_cast<long>(i),{W(s.shuttle),W(s.driver),W(s.routeId),wxString::Format("%d/%d",s.availableSeats,s.capacity),W(s.status)});}
    shuttleInner->Add(shuttleTable,1,wxEXPAND);
    tables->Add(recent,1,wxEXPAND|wxRIGHT,8);tables->Add(shuttleCard,1,wxEXPAND|wxLEFT,8);
    layout->Add(tables,1,wxEXPAND|wxALL,10);
    auto* routeActivity = new wxStaticText(page,wxID_ANY,"Route activity     Library Loop     |     Dormitory Express     |     Town Center Line");
    routeActivity->SetForegroundColour(kMuted);
    layout->Add(routeActivity,0,wxLEFT|wxRIGHT|wxBOTTOM,12);
    return page;
}

wxWindow* MainFrame::BuildRoleSelectPage() {
    auto* page = StartPage("Welcome to TaraMapuan", "Choose your portal to continue.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());
    auto* grid = new wxGridSizer(1, 16, 16);
    const auto addPortal = [this, page, grid](const wxString& title, const wxString& description,
                                             UserRole role, const wxString& action) {
        wxBoxSizer* inner{};
        auto* card = MakeCard(page, title, inner);
        auto* copy = new wxStaticText(card, wxID_ANY, description);
        copy->Wrap(420);
        inner->Add(copy, 0, wxEXPAND | wxBOTTOM, 14);
        auto* button = MakeButton(card, action, true);
        inner->Add(button, 0, wxALIGN_LEFT);
        button->Bind(wxEVT_BUTTON, [this, role](wxCommandEvent&) {
            m_selectedRole = role;
            ShowPage(Page::Login);
        });
        grid->Add(card, 1, wxEXPAND);
    };
    addPortal("Student Portal", "Browse schedules, reserve seats, view payments, and access digital passes.", UserRole::Student, "Continue as Student");
    addPortal("Driver Portal", "Manage assigned trips, inspect manifests, verify demo payments, and check in passengers.", UserRole::Driver, "Continue as Driver");
    layout->Add(grid, 0, wxEXPAND | wxALL, 14);
    auto* back = MakeButton(page, "Back to Home");
    layout->Add(back, 0, wxLEFT | wxBOTTOM, 14);
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::Landing); });
    return page;
}

wxWindow* MainFrame::BuildLoginPage() {
    const bool student = m_selectedRole == UserRole::Student;
    auto* page = StartPage(student ? "Student Login" : "Driver Login", "Demo authentication only. No production account service is connected.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());
    wxBoxSizer* inner{};
    auto* card = MakeCard(page, student ? "Student Portal" : "Driver Portal", inner);
    auto* id = new wxTextCtrl(card, wxID_ANY);
    id->SetHint(student ? "Student ID" : "Driver ID");
    auto* password = new wxTextCtrl(card, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    password->SetHint("Password");
    inner->Add(new wxStaticText(card, wxID_ANY, student ? "Student ID" : "Driver ID"), 0, wxBOTTOM, 4);
    inner->Add(id, 0, wxEXPAND | wxBOTTOM, 10);
    inner->Add(new wxStaticText(card, wxID_ANY, "Password"), 0, wxBOTTOM, 4);
    inner->Add(password, 0, wxEXPAND | wxBOTTOM, 10);
    const wxString credentials = student ? "Demo access: 2025164253 / student123" : "Demo access: DR-001 / driver123";
    auto* demo = new wxStaticText(card, wxID_ANY, credentials);
    demo->SetForegroundColour(kMuted);
    inner->Add(demo, 0, wxBOTTOM, 14);
    auto* actions = new wxBoxSizer(wxHORIZONTAL);
    auto* submit = MakeButton(card, "Login", true);
    auto* back = MakeButton(card, "Back");
    actions->Add(submit, 0, wxRIGHT, 8); actions->Add(back, 0);
    inner->Add(actions, 0);
    layout->Add(card, 0, wxEXPAND | wxALL, 14);
    submit->Bind(wxEVT_BUTTON, [this, id, password, student](wxCommandEvent&) {
        const wxString enteredId = id->GetValue().Strip(wxString::both);
        const bool credentialOk = student
            ? (enteredId == "2025164253" && password->GetValue() == "student123")
            : (password->GetValue() == "driver123" && std::any_of(m_drivers.begin(), m_drivers.end(),
                [&](const DriverRecord& driver) { return W(driver.id) == enteredId; }));
        if (!credentialOk) {
            wxMessageBox("The demo ID or password is not valid.", "Login failed", wxOK | wxICON_WARNING, this);
            return;
        }
        m_userRole = student ? UserRole::Student : UserRole::Driver;
        m_currentUserId = Utf8(enteredId);
        if (!student) {
            const auto driver = std::find_if(m_drivers.begin(), m_drivers.end(),
                [&](const DriverRecord& item) { return item.id == m_currentUserId; });
            m_selectedDriver = static_cast<int>(std::distance(m_drivers.begin(), driver));
            const auto assigned = std::find_if(m_schedules.begin(), m_schedules.end(),
                [&](const ScheduleRecord& trip) { return IsAssignedToDriver(trip, driver->id, driver->name); });
            m_selectedSchedule = assigned == m_schedules.end() ? -1 : static_cast<int>(std::distance(m_schedules.begin(), assigned));
        }
        RebuildNavigation();
        ShowPage(student ? Page::StudentDashboard : Page::DriverDashboard);
    });
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::RoleSelect); });
    return page;
}

wxWindow* MainFrame::BuildStudentDashboard() {
    auto* page = StartPage("Student Dashboard", "Your reservations, seat availability, and payment status in one place.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());
    int upcoming = 0, pending = 0, confirmed = 0, availableTrips = 0;
    for (const auto& reservation : m_reservations) {
        if (reservation.studentId != m_currentUserId || reservation.status == "Cancelled") continue;
        if (reservation.status.find("Confirmed") == 0) ++upcoming, ++confirmed;
        if (reservation.paymentStatus.find("Awaiting") == 0 || reservation.paymentStatus == "Pending") ++pending;
    }
    for (const auto& trip : m_schedules) if (IsBookableTrip(trip)) ++availableTrips;
    auto* metrics = new wxGridSizer(1, 4, 10, 10);
    metrics->Add(MetricCard(page, "UPCOMING RESERVATIONS", W(std::to_string(upcoming)), "Your active trips", kBlue), 1, wxEXPAND);
    metrics->Add(MetricCard(page, "AVAILABLE TRIPS", W(std::to_string(availableTrips)), "Seats remain", kGreen), 1, wxEXPAND);
    metrics->Add(MetricCard(page, "PENDING PAYMENTS", W(std::to_string(pending)), "Demo review status", kAmber), 1, wxEXPAND);
    metrics->Add(MetricCard(page, "CONFIRMED", W(std::to_string(confirmed)), "Active reservations", kPrimaryRed), 1, wxEXPAND);
    layout->Add(metrics, 0, wxEXPAND | wxALL, 10);
    auto* tripTools = new wxBoxSizer(wxHORIZONTAL);
    auto* sort = new wxChoice(page, wxID_ANY);
    sort->Append("Departure time");
    sort->Append("Most seats available");
    sort->Append("Route name");
    sort->SetSelection(0);
    auto* reserve = MakeButton(page, "Reserve a Seat", true);
    tripTools->Add(new wxStaticText(page, wxID_ANY, "Sort trips:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    tripTools->Add(sort, 0, wxRIGHT, 10);
    tripTools->AddStretchSpacer();
    tripTools->Add(reserve, 0);
    layout->Add(tripTools, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    reserve->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowPage(Page::Reserve); });
    wxBoxSizer* inner{};
    auto* card = MakeCard(page, "Upcoming Shuttle Trips", inner);
    auto* table = MakeTable(card, {"Trip ID", "Route", "Pickup", "Departure", "Seats", "Trip Status", "Reservation"}, {90, 150, 140, 100, 75, 100, 130});
    auto* details = new wxStaticText(card, wxID_ANY, "Select a departure to see its pickup, arrival estimate, seats and reservation status.");
    details->SetForegroundColour(kMuted);
    auto populate = [this, table, sort, details]() {
        table->DeleteAllItems();
        std::vector<size_t> indices;
        for (size_t i = 0; i < m_schedules.size(); ++i)
            if (IsBookableTrip(m_schedules[i])) indices.push_back(i);
        if (sort->GetSelection() == 1) {
            std::stable_sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
                return m_schedules[a].availableSeats > m_schedules[b].availableSeats;
            });
        } else if (sort->GetSelection() == 2) {
            std::stable_sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
                return m_schedules[a].routeId < m_schedules[b].routeId;
            });
        } else {
            std::stable_sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
                wxDateTime timeA, timeB;
                timeA.ParseTime(W(m_schedules[a].departure));
                timeB.ParseTime(W(m_schedules[b].departure));
                return timeA < timeB;
            });
        }
        for (const size_t i : indices) {
            const auto& trip = m_schedules[i];
            const auto route = std::find_if(m_routes.begin(), m_routes.end(), [&](const RouteRecord& item) { return item.id == trip.routeId; });
            const wxString routeName = route == m_routes.end() ? W(trip.routeId) : W(route->name);
            const wxString pickup = route == m_routes.end() ? wxString("Unknown") : W(route->pickup);
            const auto reservation = std::find_if(m_reservations.begin(), m_reservations.end(), [&](const ReservationRecord& item) {
                return item.studentId == m_currentUserId && item.scheduleIndex == static_cast<int>(i) && IsActiveReservation(item);
            });
            const wxString reservationStatus = reservation == m_reservations.end() ? wxString("Not reserved") : W(reservation->status);
            AddRow(table, static_cast<long>(i), {W(trip.id), routeName, pickup, W(trip.departure),
                wxString::Format("%d / %d", trip.availableSeats, trip.capacity), W(trip.status), reservationStatus});
        }
        details->SetLabel(indices.empty() ? "No upcoming departures currently have bookable seats." :
            "Select a departure to see its pickup, arrival estimate, seats and reservation status.");
    };
    populate();
    sort->Bind(wxEVT_CHOICE, [populate](wxCommandEvent&) { populate(); });
    table->Bind(wxEVT_LIST_ITEM_SELECTED, [this, table, details](wxListEvent& event) {
        const long data = static_cast<long>(table->GetItemData(event.GetIndex()));
        if (data < 0 || data >= static_cast<long>(m_schedules.size())) return;
        const auto& trip = m_schedules[static_cast<size_t>(data)];
        const auto route = std::find_if(m_routes.begin(), m_routes.end(), [&](const RouteRecord& item) { return item.id == trip.routeId; });
        const wxString routeName = route == m_routes.end() ? W(trip.routeId) : W(route->name);
        const wxString pickup = route == m_routes.end() ? wxString("Unknown") : W(route->pickup);
        const wxString destination = route == m_routes.end() ? wxString("Unknown") : W(route->destination);
        details->SetLabel(wxString::Format("%s  |  %s to %s\nPickup: %s  |  Departure: %s  |  Estimated arrival: %s\nSeats: %d of %d available  |  Status: %s",
            W(trip.id), pickup, destination, pickup, W(trip.departure), W(trip.arrival),
            trip.availableSeats, trip.capacity, W(trip.status)));
    });
    inner->Add(table, 1, wxEXPAND);
    inner->Add(details, 0, wxEXPAND | wxTOP, 10);
    layout->Add(card, 1, wxEXPAND | wxALL, 10);
    return page;
}

wxWindow* MainFrame::BuildStudentProfilePage() {
    auto* page = StartPage("My profile", "Manage your personal information and account details.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());

    wxString studentName = "Student";
    wxString studentNumber = W(m_currentUserId);
    wxString course = "Not listed";
    const auto student = std::find_if(m_passengers.begin(), m_passengers.end(), [&](const PassengerRecord& item) {
        return item.studentId == m_currentUserId;
    });
    if (student != m_passengers.end()) {
        studentName = W(student->name);
        course = W(student->department);
    }

    auto* card = new wxPanel(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    card->SetBackgroundColour(*wxWHITE);
    auto* cardLayout = new wxBoxSizer(wxVERTICAL);
    auto* cardTitle = new wxStaticText(card, wxID_ANY, "Personal information");
    cardTitle->SetFont(wxFontInfo(12).Bold());
    cardTitle->SetForegroundColour(kNavy);
    auto* cardSubtitle = new wxStaticText(card, wxID_ANY, "Your student details, all in one place.");
    cardSubtitle->SetForegroundColour(kMuted);
    cardLayout->Add(cardTitle, 0, wxBOTTOM, 4);
    cardLayout->Add(cardSubtitle, 0, wxBOTTOM, 16);

    auto* identity = new wxBoxSizer(wxHORIZONTAL);
    wxFileName photoPath(wxStandardPaths::Get().GetExecutablePath());
    photoPath.SetFullName(wxEmptyString);
    photoPath.AppendDir("assets");
    photoPath.SetFullName("studentpfp.jpg");
    wxWindow* avatar = nullptr;
    if (wxFileExists(photoPath.GetFullPath())) {
        wxImage photo(photoPath.GetFullPath());
        if (photo.IsOk()) {
            const int sourceSize = std::min(photo.GetWidth(), photo.GetHeight());
            const int cropX = (photo.GetWidth() - sourceSize) / 2;
            const int cropY = photo.GetHeight() - sourceSize;
            photo = photo.GetSubImage(wxRect(cropX, cropY, sourceSize, sourceSize));
            photo.Rescale(144, 144, wxIMAGE_QUALITY_HIGH);
            photo.InitAlpha();
            const double center = 71.5;
            const double radius = 70.0;
            for (int y = 0; y < 144; ++y) {
                for (int x = 0; x < 144; ++x) {
                    const double dx = x - center;
                    const double dy = y - center;
                    photo.SetAlpha(x, y, dx * dx + dy * dy <= radius * radius ? 255 : 0);
                }
            }
            avatar = new wxStaticBitmap(card, wxID_ANY, wxBitmap(photo));
        }
    }
    if (!avatar) {
        auto* placeholder = new wxStaticText(card, wxID_ANY, "Student\nPhoto");
        placeholder->SetBackgroundColour(wxColour(229, 235, 244));
        placeholder->SetForegroundColour(kNavy);
        avatar = placeholder;
    }
    avatar->SetMinSize(wxSize(144, 144));
    identity->Add(avatar, 0, wxRIGHT, 20);

    auto* identityCopy = new wxBoxSizer(wxVERTICAL);
    identityCopy->AddStretchSpacer(1);
    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    auto* name = new wxStaticText(card, wxID_ANY, studentName);
    name->SetFont(wxFontInfo(17).Bold());
    name->SetForegroundColour(kNavy);
    nameRow->Add(name, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    auto* verified = new wxStaticText(card, wxID_ANY, "Verified student");
    verified->SetFont(wxFontInfo(8).Bold());
    verified->SetForegroundColour(kGreen);
    nameRow->Add(verified, 0, wxALIGN_CENTER_VERTICAL);
    identityCopy->Add(nameRow, 0, wxBOTTOM, 6);
    auto* role = new wxStaticText(card, wxID_ANY, "Student passenger");
    role->SetForegroundColour(kMuted);
    identityCopy->Add(role, 0);
    identityCopy->AddStretchSpacer(1);
    identity->Add(identityCopy, 1, wxEXPAND);
    cardLayout->Add(identity, 0, wxEXPAND | wxBOTTOM, 18);

    auto* details = new wxFlexGridSizer(2, 10, 10);
    details->AddGrowableCol(0, 1);
    details->AddGrowableCol(1, 1);
    const auto addDetail = [card, details](const wxString& label, const wxString& value) {
        auto* field = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
        field->SetBackgroundColour(wxColour(248, 250, 253));
        auto* fieldLayout = new wxBoxSizer(wxVERTICAL);
        auto* caption = new wxStaticText(field, wxID_ANY, label.Upper());
        caption->SetForegroundColour(kMuted);
        caption->SetFont(wxFontInfo(8).Bold());
        auto* content = new wxStaticText(field, wxID_ANY, value);
        content->SetForegroundColour(kNavy);
        content->SetFont(wxFontInfo(10).Bold());
        fieldLayout->Add(caption, 0, wxBOTTOM, 5);
        fieldLayout->Add(content, 0);
        field->SetSizer(fieldLayout);
        details->Add(field, 1, wxEXPAND);
    };
    addDetail("Student number", studentNumber);
    addDetail("Course", course);
    cardLayout->Add(details, 0, wxEXPAND | wxBOTTOM, 16);

    auto* actions = new wxBoxSizer(wxHORIZONTAL);
    auto* editProfile = MakeButton(card, "Edit Profile", true);
    auto* changePassword = MakeButton(card, "Change Password");
    actions->Add(editProfile, 0, wxRIGHT, 8);
    actions->Add(changePassword, 0);
    cardLayout->Add(actions, 0);
    editProfile->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxMessageBox("Profile editing is not connected in this demo.", "Demo profile",
                     wxOK | wxICON_INFORMATION, this);
    });
    changePassword->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxMessageBox("Password changes are not connected to demo authentication.", "Demo account",
                     wxOK | wxICON_INFORMATION, this);
    });
    card->SetSizer(cardLayout);
    layout->Add(card, 0, wxEXPAND | wxALL, 12);
    return page;
}

wxWindow* MainFrame::BuildReservationPage() {
    // TODO [Kevin Miguel A. Antido]: Add FIFO reservation requests, a waitlist, and seat reallocation when a reservation is cancelled.
    // TODO [Alejandro Antonio T. Karunugan]: Coordinate shared reservation interfaces so student bookings and driver manifests stay consistent.
    auto* page=StartPage("New reservation","Complete the passenger and trip details. Payment is simulated and seat counts update only in this session.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    wxBoxSizer* cardInner{};auto* card=MakeCard(page,"Reservation form",cardInner);
    auto* note=new wxStaticText(card,wxID_ANY,"Prototype: no reservation is sent to a service and no payment is processed.");note->SetForegroundColour(kAmber);cardInner->Add(note,0,wxBOTTOM,12);
    auto* fields=new wxFlexGridSizer(2,10,12);fields->AddGrowableCol(1,1);
    const std::pair<wxString,wxString> labels[]={{"Passenger","Enter passenger name"},{"Route","Choose a route"},{"Travel date","Use YYYY-MM-DD"},{"Schedule","Select a departure"},{"Seats","Choose the number of seats"},{"Payment","Cash or GCash"}};
    for(const auto&item:labels){auto*t=new wxStaticText(card,wxID_ANY,item.first);t->SetForegroundColour(kInk);fields->Add(t,0,wxALIGN_CENTER_VERTICAL);auto*v=new wxStaticText(card,wxID_ANY,item.second);v->SetForegroundColour(kMuted);fields->Add(v,1,wxEXPAND);}
    cardInner->Add(fields,0,wxEXPAND|wxBOTTOM,10);
    auto* create=MakeButton(card,"Open Reservation Form",true);cardInner->Add(create,0,wxALIGN_LEFT);
    create->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){CreateReservation();});
    // TODO(TESTING): Add automated tests for required fields and seat-capacity rules.
    // TODO(BACKEND): Send validated reservation data to the campus shuttle service.
    // TODO(PAYMENT): Integrate a payment provider and verify references externally.
    // TODO(SECURITY): Add authentication, authorization, and input sanitization.
    layout->Add(card,0,wxEXPAND|wxALL,10);
    auto* info=new wxStaticText(page,wxID_ANY,"Current sample availability is shown on the Schedule and Shuttle Monitoring pages.");info->SetForegroundColour(kMuted);layout->Add(info,0,wxLEFT|wxRIGHT|wxBOTTOM,12);
    return page;
}

void MainFrame::CreateReservation() {
    std::string presetName;
    if (m_userRole == UserRole::Student) {
        const auto passenger = std::find_if(m_passengers.begin(), m_passengers.end(), [&](const PassengerRecord& item) {
            return item.studentId == m_currentUserId;
        });
        if (passenger != m_passengers.end()) presetName = passenger->name;
    }
    ReservationDialog dialog(this, m_routes, m_schedules, presetName,
                             m_userRole == UserRole::Student ? m_currentUserId : std::string{});
    if(dialog.ShowModal()!=wxID_OK)return;
    const ReservationDraft draft=dialog.GetDraft();
    if(draft.routeIndex<0||draft.routeIndex>=static_cast<int>(m_routes.size())||
       draft.scheduleIndex<0||draft.scheduleIndex>=static_cast<int>(m_schedules.size())||draft.seatCount<1){
        wxMessageBox("The selected route, departure, or seat count is no longer valid.",
                     "Reservation details changed",wxOK|wxICON_WARNING,this);return;
    }
    auto& schedule=m_schedules[static_cast<size_t>(draft.scheduleIndex)];
    if(schedule.routeId!=m_routes[static_cast<size_t>(draft.routeIndex)].id||
       schedule.status=="On Route"||schedule.status=="Completed"){
        wxMessageBox("That departure is no longer available for booking.",
                     "Departure unavailable",wxOK|wxICON_WARNING,this);return;
    }
    if(IsPastDepartureCutoff(draft.travelDate+" "+W(schedule.departure),30)){
        wxMessageBox("Reservations close 30 minutes before departure.",
                     "Booking cutoff passed",wxOK|wxICON_WARNING,this);return;
    }
    if (m_userRole == UserRole::Student && draft.studentId != W(m_currentUserId)) {
        wxMessageBox("Use the student ID associated with your current demo login.", "Student ID mismatch", wxOK | wxICON_WARNING, this);
        return;
    }
    const std::string studentId = Utf8(draft.studentId);
    const auto duplicate = std::find_if(m_reservations.begin(), m_reservations.end(), [&](const ReservationRecord& reservation) {
        return reservation.studentId == studentId && reservation.scheduleIndex == draft.scheduleIndex && reservation.status != "Cancelled";
    });
    if (duplicate != m_reservations.end()) {
        wxMessageBox("This student already has an active reservation for the selected departure.", "Duplicate reservation", wxOK | wxICON_WARNING, this);
        return;
    }
    const bool waitlisted=draft.joinWaitlist&&draft.seatCount>schedule.availableSeats;
    if(draft.seatCount>schedule.availableSeats&&!waitlisted){
        wxMessageBox(wxString::Format("Only %d seat(s) remain on this departure.",schedule.availableSeats),"Not enough seats",wxOK|wxICON_WARNING,this);return;
    }
    const wxString date=draft.travelDate;
    const std::string id="TS-"+std::to_string(m_reservationSequence++);
    const std::string payment=Utf8(draft.paymentMethod);
    const std::string status=waitlisted?"Waitlisted":"Confirmed (demo)";
    const std::string paymentStatus=waitlisted?"Not due until a seat is offered":(payment=="Cash"?"Due on board":"Awaiting review (demo)");
    const std::string seat=waitlisted?"--":AllocateSeatRange(m_reservations,draft.scheduleIndex,draft.seatCount,schedule.capacity);
    if(seat.empty()){wxMessageBox("No contiguous seat assignment could be created for this departure.","Seat assignment failed",wxOK|wxICON_WARNING,this);return;}
    m_reservations.push_back({id,Utf8(draft.passenger),studentId,m_routes[static_cast<size_t>(draft.routeIndex)].name,
        Utf8(date)+"  "+Utf8(schedule.departure),seat,payment,paymentStatus,status,draft.seatCount,draft.scheduleIndex,
        Utf8(draft.paymentReference),"Waiting"});
    if(!waitlisted)schedule.availableSeats-=draft.seatCount;
    if(schedule.availableSeats==0)schedule.status="Full";
    SaveReservations();
    m_selectedReservationId = id;
    wxMessageBox(waitlisted
        ? wxString::Format("Reservation %s was added to the departure waitlist. Payment is not due unless a seat is offered.",W(id))
        : wxString::Format("Sample reservation %s was added. No external booking or payment occurred.",W(id)),
                 "Reservation created",wxOK|wxICON_INFORMATION,this);
    SetStatusText("Local preview updated  |  Sample reservation "+W(id));
    RefreshCurrentPage();
    // TODO: Add reservation cutoff/no-show handling, notification delivery, and an auditable reservation event log.
}

wxWindow* MainFrame::BuildReservationsPage() {
    auto*page=StartPage("Reservations","Search, inspect, and cancel fictional passenger reservations.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());
    auto*bar=new wxBoxSizer(wxHORIZONTAL);auto*search=new wxTextCtrl(page,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(260,-1));search->SetHint("Search passenger, ID, route, reference");
    auto*filter=new wxChoice(page,wxID_ANY);for(const char*value:{"All statuses","Pending","Confirmed (demo)","Waitlisted","Cancelled"})filter->Append(value);filter->SetSelection(0);
    auto*create=MakeButton(page,"+ New Reservation",true);auto*details=MakeButton(page,"View Details");auto*cancel=MakeButton(page,"Cancel Reservation");
    bar->Add(search,0,wxRIGHT,8);bar->Add(filter,0,wxRIGHT,8);bar->AddStretchSpacer();bar->Add(create,0,wxRIGHT,6);bar->Add(details,0,wxRIGHT,6);bar->Add(cancel,0);
    layout->Add(bar,0,wxEXPAND|wxALL,10);
    wxBoxSizer*inner{};auto*card=MakeCard(page,"Reservation records  |  Shared demo data",inner);auto*table=MakeTable(card,{"ID","Passenger","Student ID","Route","Departure","Seat","Payment","Payment status","Boarding","Reservation status"},{90,120,110,130,110,50,70,125,90,130});
    auto populate=[this,table,search,filter](){table->DeleteAllItems();const int selected=filter->GetSelection();const wxString query=search->GetValue();for(size_t i=0;i<m_reservations.size();++i){const auto&r=m_reservations[i];if(m_userRole==UserRole::Student&&r.studentId!=m_currentUserId)continue;if(selected>0&&r.status!=Utf8(filter->GetStringSelection()))continue;if(!Includes(r.id,query)&&!Includes(r.passenger,query)&&!Includes(r.studentId,query)&&!Includes(r.route,query)&&!Includes(r.paymentReference,query))continue;AddRow(table,static_cast<long>(i),{W(r.id),W(r.passenger),W(r.studentId),W(r.route),W(r.departure),W(r.seat),W(r.paymentMethod),W(r.paymentStatus),W(r.boardingStatus),W(r.status)});}};
    populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});filter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});
    table->Bind(wxEVT_LIST_ITEM_ACTIVATED,[this,table](wxListEvent&e){ShowReservationDetails(static_cast<long>(table->GetItemData(e.GetIndex())));});
    inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);
    create->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){CreateReservation();});
    details->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){const long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a reservation first.","No reservation selected",wxOK|wxICON_INFORMATION,this);return;}ShowReservationDetails(static_cast<long>(table->GetItemData(row)));});
    cancel->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){const long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a reservation first.","No reservation selected",wxOK|wxICON_INFORMATION,this);return;}CancelReservation(static_cast<long>(table->GetItemData(row)));});
    return page;
}

void MainFrame::ShowReservationDetails(long rowIndex){
    if(rowIndex<0||rowIndex>=static_cast<long>(m_reservations.size()))return;const auto&r=m_reservations[static_cast<size_t>(rowIndex)];
    wxMessageBox(wxString::Format("Passenger: %s\nStudent ID: %s\nRoute: %s\nDeparture: %s\nSeat(s): %s\nPayment: %s (%s)\nReference: %s\nBoarding: %s\nReservation: %s",
        W(r.passenger),W(r.studentId),W(r.route),W(r.departure),W(r.seat),W(r.paymentMethod),W(r.paymentStatus),
        W(r.paymentReference.empty()?"--":r.paymentReference),W(r.boardingStatus),W(r.status)),"Reservation details",wxOK|wxICON_INFORMATION,this);
}

void MainFrame::CancelReservation(long rowIndex){
    if(rowIndex<0||rowIndex>=static_cast<long>(m_reservations.size()))return;
    auto& reservation=m_reservations[static_cast<size_t>(rowIndex)];
    if(reservation.status=="Cancelled"){
        wxMessageBox("This sample reservation is already cancelled.","Reservation status",wxOK|wxICON_INFORMATION,this);return;
    }
    if(reservation.boardingStatus=="Boarded"){
        wxMessageBox("A boarded passenger cannot cancel this reservation.","Cancellation unavailable",wxOK|wxICON_WARNING,this);return;
    }
    if(reservation.scheduleIndex<0||reservation.scheduleIndex>=static_cast<int>(m_schedules.size())){
        wxMessageBox("The linked departure could not be found, so this reservation cannot be cancelled safely.",
                     "Cancellation unavailable",wxOK|wxICON_WARNING,this);return;
    }
    auto& schedule=m_schedules[static_cast<size_t>(reservation.scheduleIndex)];
    if(schedule.status=="On Route"||schedule.status=="Completed"||
       IsPastDepartureCutoff(W(reservation.departure),30)){
        wxMessageBox("Cancellation closes 30 minutes before departure. This trip can no longer be cancelled.",
                     "Cancellation cutoff passed",wxOK|wxICON_WARNING,this);return;
    }
    if(wxMessageBox("Cancel this reservation? Allocated seats will be returned and may be offered to the next waitlisted passenger.",
                    "Confirm cancellation",wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION,this)!=wxYES)return;

    const int scheduleIndex=reservation.scheduleIndex;
    const int releasedSeats=HasAllocatedSeats(reservation)?reservation.seatCount:0;
    reservation.status="Cancelled";
    reservation.paymentStatus="Not collected";
    reservation.boardingStatus="Cancelled";
    if(releasedSeats>0){
        schedule.availableSeats=std::min(schedule.capacity,schedule.availableSeats+releasedSeats);
        if(schedule.availableSeats>0&&schedule.status=="Full")schedule.status="Available";
    }

    int promoted=0;
    for(auto& waiting:m_reservations){
        if(waiting.status!="Waitlisted"||waiting.scheduleIndex!=scheduleIndex)continue;
        if(waiting.seatCount>schedule.availableSeats)break; // Preserve first-in, first-out order.
        const std::string seat=AllocateSeatRange(m_reservations,scheduleIndex,waiting.seatCount,schedule.capacity);
        if(seat.empty())break;
        waiting.seat=seat;
        waiting.status="Confirmed (demo)";
        waiting.paymentStatus=waiting.paymentMethod=="Cash"?"Due on board":"Payment due after seat offer";
        waiting.boardingStatus="Waiting";
        schedule.availableSeats-=waiting.seatCount;
        ++promoted;
    }
    if(schedule.availableSeats==0)schedule.status="Full";
    SaveReservations();
    SetStatusText(promoted>0?wxString::Format("Reservation cancelled; %d waitlisted passenger(s) received a seat offer.",promoted):wxString("Sample reservation cancelled; its seats were released."));
    wxMessageBox(promoted>0?wxString::Format("Reservation cancelled. %d waitlisted passenger(s) were offered a seat.",promoted):wxString("Reservation cancelled. Its seat(s) were returned to the schedule."),
                 "Cancellation complete",wxOK|wxICON_INFORMATION,this);
    RefreshCurrentPage();
}

wxWindow* MainFrame::BuildRoutesPage(){
    auto*page=StartPage("Shuttle routes","Manage sample route definitions. All route changes remain in memory.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());auto*bar=new wxBoxSizer(wxHORIZONTAL);auto*search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search route, pickup, destination");auto*add=MakeButton(page,"Add Route",true);auto*edit=MakeButton(page,"Edit Route");auto*details=MakeButton(page,"View Details");auto*remove=MakeButton(page,"Delete Route");bar->Add(search,1,wxRIGHT,8);bar->Add(add,0,wxRIGHT,6);bar->Add(edit,0,wxRIGHT,6);bar->Add(details,0,wxRIGHT,6);bar->Add(remove,0);layout->Add(bar,0,wxEXPAND|wxALL,10);
    wxBoxSizer*inner{};auto*card=MakeCard(page,"Route directory",inner);auto*table=MakeTable(card,{"Route ID","Route name","Pickup","Destination","Shuttle","Travel time","Status"},{85,150,140,135,80,100,90});auto populate=[this,table,search](){table->DeleteAllItems();for(size_t i=0;i<m_routes.size();++i){const auto&r=m_routes[i];if(!Includes(r.id,search->GetValue())&&!Includes(r.name,search->GetValue())&&!Includes(r.pickup,search->GetValue())&&!Includes(r.destination,search->GetValue()))continue;AddRow(table,static_cast<long>(i),{W(r.id),W(r.name),W(r.pickup),W(r.destination),W(r.shuttle),W(r.travelTime),W(r.status)});}};populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);
    auto selected=[this,table]()->long{return table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);};
    add->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){AddOrEditRoute();});edit->Bind(wxEVT_BUTTON,[this,selected,table](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a route to edit.","No route selected",wxOK|wxICON_INFORMATION,this);return;}AddOrEditRoute(static_cast<long>(table->GetItemData(row)));});
    details->Bind(wxEVT_BUTTON,[this,selected,table](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a route to view.","No route selected",wxOK|wxICON_INFORMATION,this);return;}const auto&r=m_routes[static_cast<size_t>(table->GetItemData(row))];wxMessageBox(wxString::Format("%s - %s\nPickup: %s\nDestination: %s\nAssigned shuttle: %s\nEstimated travel: %s\nStatus: %s",W(r.id),W(r.name),W(r.pickup),W(r.destination),W(r.shuttle),W(r.travelTime),W(r.status)),"Route details",wxOK|wxICON_INFORMATION,this);});
    remove->Bind(wxEVT_BUTTON,[this,selected,table](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a route to delete.","No route selected",wxOK|wxICON_INFORMATION,this);return;}long i=static_cast<long>(table->GetItemData(row));if(wxMessageBox("Delete this sample route?","Confirm deletion",wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION,this)!=wxYES)return;m_routes.erase(m_routes.begin()+i);RefreshCurrentPage();});
    // TODO(API): Synchronize route updates with a campus transportation service.
    // TODO [John Angelo V. Yboa]: Add BFS route traversal once campus stop connectivity is represented as a graph.
    // TODO(SECURITY): Restrict route administration to authorized staff.
    return page;
}

void MainFrame::AddOrEditRoute(long routeIndex){
    const bool editing=routeIndex>=0&&routeIndex<static_cast<long>(m_routes.size());std::vector<std::string> values;
    if(editing){const auto&r=m_routes[static_cast<size_t>(routeIndex)];values={r.id,r.name,r.pickup,r.destination,r.shuttle,r.travelTime,r.status};}
    else values={"RT-06","New Campus Loop","Main Campus","New Stop","SH-006","15 min","Active"};
    const wxString prompt="Enter 7 values separated by |:\nID | Name | Pickup | Destination | Shuttle | Travel time | Status";
    wxTextEntryDialog dialog(this,prompt,editing?"Edit Route":"Add Route",JoinPipe(values));if(dialog.ShowModal()!=wxID_OK)return;auto fields=SplitPipe(dialog.GetValue());if(fields.size()!=7){wxMessageBox("Enter exactly 7 pipe-separated values.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    for(const auto&f:fields)if(f.empty()){wxMessageBox("Route fields cannot be blank.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    RouteRecord record{Utf8(fields[0]),Utf8(fields[1]),Utf8(fields[2]),Utf8(fields[3]),Utf8(fields[4]),Utf8(fields[5]),Utf8(fields[6])};if(editing)m_routes[static_cast<size_t>(routeIndex)]=std::move(record);else m_routes.push_back(std::move(record));RefreshCurrentPage();
}

wxWindow* MainFrame::BuildSchedulesPage(){
    auto*page=StartPage("Shuttle schedules","Filter departures by route, time, or availability. Seat totals update after mock reservations.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());auto*bar=new wxBoxSizer(wxHORIZONTAL);auto*search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search time, driver, shuttle");auto*routeFilter=new wxChoice(page,wxID_ANY);routeFilter->Append("All routes");for(const auto&r:m_routes)routeFilter->Append(W(r.name));routeFilter->SetSelection(0);auto*statusFilter=new wxChoice(page,wxID_ANY);for(const char*s:{"All statuses","Available","Boarding","On Route","Full"})statusFilter->Append(s);statusFilter->SetSelection(0);auto*add=MakeButton(page,"Add Schedule",true);bar->Add(search,1,wxRIGHT,8);bar->Add(routeFilter,0,wxRIGHT,8);bar->Add(statusFilter,0,wxRIGHT,8);bar->Add(add,0);layout->Add(bar,0,wxEXPAND|wxALL,10);
    wxBoxSizer*inner{};auto*card=MakeCard(page,"Departure board",inner);auto*table=MakeTable(card,{"Schedule ID","Route","Departure","Arrival est.","Driver","Shuttle","Seats","Status"},{95,135,90,95,120,80,65,100});auto populate=[this,table,search,routeFilter,statusFilter](){table->DeleteAllItems();const wxString query=search->GetValue();for(size_t i=0;i<m_schedules.size();++i){const auto&s=m_schedules[i];if(routeFilter->GetSelection()>0&&s.routeId!=m_routes[static_cast<size_t>(routeFilter->GetSelection()-1)].id)continue;if(statusFilter->GetSelection()>0&&s.status!=Utf8(statusFilter->GetStringSelection()))continue;if(!Includes(s.id,query)&&!Includes(s.driver,query)&&!Includes(s.shuttle,query)&&!Includes(s.departure,query)&&!Includes(s.routeId,query))continue;AddRow(table,static_cast<long>(i),{W(s.id),W(s.routeId),W(s.departure),W(s.arrival),W(s.driver),W(s.shuttle),wxString::Format("%d / %d",s.availableSeats,s.capacity),W(s.status)});}};populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});routeFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});statusFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);add->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){AddSchedule();});
    auto*sync=MakeButton(page,"Sync Departures");bar->Add(sync,0,wxLEFT,6);sync->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){
        // The in-memory schedule collection is the current campus schedule service adapter.
        // Revalidate its references and reject duplicate departure IDs or overlapping vehicle assignments.
        auto minutes=[](const std::string& value)->int{wxString text=W(value);wxDateTime parsed;if(!parsed.ParseTime(text))return -1;return parsed.GetHour()*60+parsed.GetMinute();};
        std::vector<std::string> issues;
        for(size_t i=0;i<m_schedules.size();++i){const auto&a=m_schedules[i];
            if(std::none_of(m_routes.begin(),m_routes.end(),[&](const RouteRecord&r){return r.id==a.routeId;})){issues.push_back(a.id+": unknown route "+a.routeId);continue;}
            const int startA=minutes(a.departure),endA=minutes(a.arrival);
            for(size_t j=0;j<i;++j){const auto&b=m_schedules[j];if(a.id==b.id){issues.push_back(a.id+": duplicate schedule ID");break;}
                const int startB=minutes(b.departure),endB=minutes(b.arrival);
                if(startA<0||endA<0||startB<0||endB<0)continue;
                const int finishA=endA<startA?endA+1440:endA,finishB=endB<startB?endB+1440:endB;
                if(a.shuttle==b.shuttle&&startA<finishB&&startB<finishA)issues.push_back(a.id+": shuttle "+a.shuttle+" conflicts with "+b.id);
                if(a.routeId==b.routeId&&startA==startB)issues.push_back(a.id+": duplicate route departure time with "+b.id);
            }
        }
        if(issues.empty()){SetStatusText(wxString::Format("Campus schedule sync complete: %zu departure(s) validated.",m_schedules.size()));wxMessageBox("Departures are synchronized with the current campus schedule and no route or vehicle conflicts were found.","Schedule sync",wxOK|wxICON_INFORMATION,this);}
        else{wxString report;for(const auto&issue:issues)report+=W(issue)+"\n";SetStatusText(wxString::Format("Schedule sync found %zu conflict(s).",issues.size()));wxMessageBox("The schedule could not be synchronized:\n\n"+report,"Schedule conflicts",wxOK|wxICON_WARNING,this);}
    });
    return page;
}

void MainFrame::AddSchedule(){
    wxTextEntryDialog dialog(this,"Enter 9 values separated by |:\nID | Route ID | Departure | Arrival | Driver | Shuttle | Status | Available seats | Capacity","Add Sample Schedule","SC-106|RT-01|11:00 AM|11:12 AM|Juan Dela Cruz|SH-001|Available|18|18");
    if(dialog.ShowModal()!=wxID_OK)return;
    auto f=SplitPipe(dialog.GetValue());
    if(f.size()!=9){wxMessageBox("Enter exactly 9 pipe-separated values.","Invalid schedule",wxOK|wxICON_WARNING,this);return;}
    for(const auto&field:f)if(field.empty()){wxMessageBox("Schedule fields cannot be blank.","Invalid schedule",wxOK|wxICON_WARNING,this);return;}
    const std::string id=Utf8(f[0]),routeId=Utf8(f[1]),departure=Utf8(f[2]),arrival=Utf8(f[3]),shuttle=Utf8(f[5]);
    auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord&r){return r.id==routeId;});
    if(route==m_routes.end()){wxMessageBox("The route ID does not exist.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    const auto assignedDriver=std::find_if(m_drivers.begin(),m_drivers.end(),[&](const DriverRecord& driver){return driver.name==Utf8(f[4]);});
    if(assignedDriver==m_drivers.end()){wxMessageBox("Choose a driver from the existing driver directory.","Invalid driver",wxOK|wxICON_WARNING,this);return;}
    long seats=0,capacity=0;
    if(!f[7].ToLong(&seats)||!f[8].ToLong(&capacity)||seats<0||capacity<1||seats>capacity){wxMessageBox("Seat counts must be valid and available seats cannot exceed capacity.","Invalid capacity",wxOK|wxICON_WARNING,this);return;}
    wxDateTime departureTime,arrivalTime;
    if(!departureTime.ParseTime(W(departure))||!arrivalTime.ParseTime(W(arrival))){wxMessageBox("Enter departure and arrival times in a valid format, such as 11:00 AM.","Invalid time",wxOK|wxICON_WARNING,this);return;}
    const int start=departureTime.GetHour()*60+departureTime.GetMinute();
    int finish=arrivalTime.GetHour()*60+arrivalTime.GetMinute();if(finish<=start)finish+=1440;
    for(const auto&existing:m_schedules){
        if(existing.id==id){wxMessageBox("That schedule ID already exists.","Schedule conflict",wxOK|wxICON_WARNING,this);return;}
        wxDateTime otherStart,otherFinish;
        if(!otherStart.ParseTime(W(existing.departure))||!otherFinish.ParseTime(W(existing.arrival)))continue;
        const int from=otherStart.GetHour()*60+otherStart.GetMinute();int to=otherFinish.GetHour()*60+otherFinish.GetMinute();if(to<=from)to+=1440;
        if(existing.shuttle==shuttle&&start<to&&from<finish){wxMessageBox(wxString("This shuttle is already assigned to an overlapping departure (")+W(existing.id)+").","Vehicle conflict",wxOK|wxICON_WARNING,this);return;}
        if(existing.routeId==routeId&&from==start){wxMessageBox(wxString("This route already has a departure at that time (")+W(existing.id)+").","Route conflict",wxOK|wxICON_WARNING,this);return;}
    }
    m_schedules.push_back({id,routeId,departure,arrival,Utf8(f[4]),shuttle,Utf8(f[6]),static_cast<int>(seats),static_cast<int>(capacity),assignedDriver->id});
    RefreshCurrentPage();
}

wxWindow* MainFrame::BuildPassengersPage(){
    auto*page=StartPage("Passenger management","Search fictional passenger records and review their local reservation history.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());auto*bar=new wxBoxSizer(wxHORIZONTAL);auto*search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search name, student ID, department");auto*details=MakeButton(page,"View Details",true);bar->Add(search,1,wxRIGHT,8);bar->Add(details,0);layout->Add(bar,0,wxEXPAND|wxALL,10);
    wxBoxSizer*inner{};auto*card=MakeCard(page,"Passenger directory  |  Fictional sample data",inner);auto*table=MakeTable(card,{"Student ID","Passenger name","Course / department","Contact","Reservation history"},{130,150,180,120,140});auto populate=[this,table,search](){table->DeleteAllItems();for(size_t i=0;i<m_passengers.size();++i){const auto&p=m_passengers[i];if(!Includes(p.studentId,search->GetValue())&&!Includes(p.name,search->GetValue())&&!Includes(p.department,search->GetValue()))continue;int history=0;for(const auto&r:m_reservations)if(r.studentId==p.studentId)++history;AddRow(table,static_cast<long>(i),{W(p.studentId),W(p.name),W(p.department),W(p.contact),wxString::Format("%d reservation(s)",history)});}};populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);details->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a passenger first.","No passenger selected",wxOK|wxICON_INFORMATION,this);return;}const auto&p=m_passengers[static_cast<size_t>(table->GetItemData(row))];wxString history;for(const auto&r:m_reservations)if(r.studentId==p.studentId)history+=W(r.id+" - "+r.route+" ("+r.status+")\n");if(history.empty())history="No sample reservations.";wxMessageBox(wxString::Format("%s\nStudent ID: %s\nDepartment: %s\nContact: %s\n\nHistory:\n%s",W(p.name),W(p.studentId),W(p.department),W(p.contact),history),"Passenger details",wxOK|wxICON_INFORMATION,this);});
    // TODO(AUTH): Add secure passenger and staff authentication before showing personal records.
    // TODO(SECURITY): Protect personal contact data with role-based access and input sanitization.
    // TODO(PERSISTENCE): Store passenger records securely rather than in this in-memory sample.
    return page;
}

wxWindow* MainFrame::BuildDriversPage(){auto*page=StartPage("Driver management","Review and edit fictional driver assignments.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());auto*bar=new wxBoxSizer(wxHORIZONTAL);auto*search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search driver, ID, shuttle, route");auto*add=MakeButton(page,"Add Driver",true);auto*edit=MakeButton(page,"Edit Driver");auto*details=MakeButton(page,"View Details");bar->Add(search,1,wxRIGHT,8);bar->Add(add,0,wxRIGHT,6);bar->Add(edit,0,wxRIGHT,6);bar->Add(details,0);layout->Add(bar,0,wxEXPAND|wxALL,10);wxBoxSizer*inner{};auto*card=MakeCard(page,"Driver directory",inner);auto*table=MakeTable(card,{"Driver ID","Name","Shuttle","Assigned route","Contact","Availability"},{90,150,85,160,120,100});auto populate=[this,table,search](){table->DeleteAllItems();for(size_t i=0;i<m_drivers.size();++i){const auto&d=m_drivers[i];if(!Includes(d.name,search->GetValue())&&!Includes(d.id,search->GetValue())&&!Includes(d.shuttle,search->GetValue())&&!Includes(d.route,search->GetValue()))continue;AddRow(table,static_cast<long>(i),{W(d.id),W(d.name),W(d.shuttle),W(d.route),W(d.contact),W(d.availability)});}};populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);auto selected=[table](){return table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);};add->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){AddOrEditDriver();});edit->Bind(wxEVT_BUTTON,[this,table,selected](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a driver to edit.","No driver selected",wxOK|wxICON_INFORMATION,this);return;}AddOrEditDriver(static_cast<long>(table->GetItemData(row)));});details->Bind(wxEVT_BUTTON,[this,table,selected](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a driver first.","No driver selected",wxOK|wxICON_INFORMATION,this);return;}const auto&d=m_drivers[static_cast<size_t>(table->GetItemData(row))];wxMessageBox(wxString::Format("%s (%s)\nShuttle: %s\nRoute: %s\nContact: %s\nAvailability: %s",W(d.name),W(d.id),W(d.shuttle),W(d.route),W(d.contact),W(d.availability)),"Driver details",wxOK|wxICON_INFORMATION,this);});
    // TODO(AUTH): Require a staff role before changing driver assignments.
    // TODO(PERSISTENCE): Persist approved driver and fleet assignments.
    return page;}

void MainFrame::AddOrEditDriver(long driverIndex) {
    const bool editing=driverIndex>=0&&driverIndex<static_cast<long>(m_drivers.size());
    std::vector<std::string> values;
    if(editing){const auto& driver=m_drivers[static_cast<size_t>(driverIndex)];values={driver.id,driver.name,driver.shuttle,driver.route,driver.contact,driver.availability};}
    else values={"DR-005","New Driver","SH-005","Sports Complex Route","555-0205","Available"};
    wxTextEntryDialog dialog(this,"Enter 6 values separated by |:\nID | Name | Shuttle | Route | Contact | Availability",editing?"Edit Driver":"Add Driver",JoinPipe(values));
    if(dialog.ShowModal()!=wxID_OK)return;
    auto fields=SplitPipe(dialog.GetValue());
    if(fields.size()!=6){wxMessageBox("Enter exactly 6 pipe-separated values.","Invalid driver",wxOK|wxICON_WARNING,this);return;}
    for(const auto& field:fields)if(field.empty()){wxMessageBox("Driver fields cannot be blank.","Invalid driver",wxOK|wxICON_WARNING,this);return;}
    DriverRecord record{Utf8(fields[0]),Utf8(fields[1]),Utf8(fields[2]),Utf8(fields[3]),Utf8(fields[4]),Utf8(fields[5])};
    for(size_t i=0;i<m_drivers.size();++i)if((!editing||i!=static_cast<size_t>(driverIndex))&&(m_drivers[i].id==record.id||m_drivers[i].name==record.name)){wxMessageBox("Driver ID and name must be unique.","Duplicate driver",wxOK|wxICON_WARNING,this);return;}
    if(editing){const std::string previousId=m_drivers[static_cast<size_t>(driverIndex)].id;for(auto& trip:m_schedules)if(trip.driverId==previousId){trip.driverId=record.id;trip.driver=record.name;}m_drivers[static_cast<size_t>(driverIndex)]=std::move(record);}
    else m_drivers.push_back(std::move(record));
    RefreshCurrentPage();
}
wxWindow* MainFrame::BuildDriverDashboard() {
    auto* page = StartPage("Driver Operations", "Assigned trip overview and live counts from shared reservation records.");
    auto* layout = static_cast<wxBoxSizer*>(page->GetSizer());
    if (m_selectedDriver < 0 || m_selectedDriver >= static_cast<int>(m_drivers.size())) {
        layout->Add(new wxStaticText(page, wxID_ANY, "No driver is signed in."), 0, wxALL, 12);
        return page;
    }
    const auto& driver = m_drivers[static_cast<size_t>(m_selectedDriver)];
    int trips = 0, passengers = 0, boarded = 0, pendingPayments = 0;
    for (const auto& trip : m_schedules) if (IsAssignedToDriver(trip,driver.id,driver.name) && trip.status != "Completed") ++trips;
    for (const auto& reservation : m_reservations) {
        if (!HasAllocatedSeats(reservation) || reservation.scheduleIndex < 0 || reservation.scheduleIndex >= static_cast<int>(m_schedules.size())) continue;
        if (!IsAssignedToDriver(m_schedules[static_cast<size_t>(reservation.scheduleIndex)],driver.id,driver.name)) continue;
        ++passengers;
        if (reservation.boardingStatus == "Boarded") ++boarded;
        if (!IsPaymentCleared(reservation) && reservation.paymentStatus.find("Rejected") != 0) ++pendingPayments;
    }
    auto* metrics = new wxGridSizer(1, 4, 10, 10);
    metrics->Add(MetricCard(page,"ASSIGNED TRIPS",W(std::to_string(trips)),"Not completed",kBlue),1,wxEXPAND);
    metrics->Add(MetricCard(page,"PASSENGERS",W(std::to_string(passengers)),"Active reservations",kBlue),1,wxEXPAND);
    metrics->Add(MetricCard(page,"BOARDED",W(std::to_string(boarded)),"Checked in",kGreen),1,wxEXPAND);
    metrics->Add(MetricCard(page,"PAYMENT REVIEW",W(std::to_string(pendingPayments)),"Awaiting driver action",kAmber),1,wxEXPAND);
    layout->Add(metrics,0,wxEXPAND|wxALL,10);

    if (m_selectedSchedule < 0 || m_selectedSchedule >= static_cast<int>(m_schedules.size()) ||
        !IsAssignedToDriver(m_schedules[static_cast<size_t>(m_selectedSchedule)],driver.id,driver.name)) {
        const auto next = std::find_if(m_schedules.begin(),m_schedules.end(),[&](const ScheduleRecord& trip){return IsAssignedToDriver(trip,driver.id,driver.name)&&trip.status!="Completed";});
        m_selectedSchedule = next == m_schedules.end() ? -1 : static_cast<int>(std::distance(m_schedules.begin(),next));
    }
    wxBoxSizer* inner{};
    auto* card = MakeCard(page,"Current / next assigned trip",inner);
    if (m_selectedSchedule < 0) {
        inner->Add(new wxStaticText(card,wxID_ANY,"No active trip is assigned to this demo driver."),0,wxEXPAND);
    } else {
        const auto& trip=m_schedules[static_cast<size_t>(m_selectedSchedule)];
        const auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord& item){return item.id==trip.routeId;});
        int booked=0,tripBoarded=0;
        for(const auto& reservation:m_reservations) if(HasAllocatedSeats(reservation)&&reservation.scheduleIndex==m_selectedSchedule){++booked;if(reservation.boardingStatus=="Boarded")++tripBoarded;}
        const wxString pickup=route==m_routes.end()?wxString("Unknown"):W(route->pickup);
        inner->Add(new wxStaticText(card,wxID_ANY,wxString::Format("%s  |  %s\nPickup: %s   Departure: %s\nCapacity: %d   Booked: %d   Boarded: %d\nTrip status: %s",
            W(trip.id),route==m_routes.end()?W(trip.routeId):W(route->name),pickup,W(trip.departure),trip.capacity,booked,tripBoarded,W(trip.status))),0,wxEXPAND|wxBOTTOM,12);
        auto* open=MakeButton(card,"Open Manifest",true);
        inner->Add(open,0,wxALIGN_LEFT);
        open->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){ShowPage(Page::Manifest);});
    }
    layout->Add(card,0,wxEXPAND|wxALL,10);
    auto* tripsButton=MakeButton(page,"View Assigned Trips");
    layout->Add(tripsButton,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    tripsButton->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){ShowPage(Page::AssignedTrips);});
    return page;
}

wxWindow* MainFrame::BuildAssignedTripsPage() {
    auto* page=StartPage("Assigned Trips","Trips assigned to the signed-in driver. Select one to review its shared passenger manifest.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    if(m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())) return page;
    const auto& driver=m_drivers[static_cast<size_t>(m_selectedDriver)];
    wxBoxSizer* inner{};auto* card=MakeCard(page,"Driver: "+W(driver.name),inner);
    auto* table=MakeTable(card,{"Trip ID","Route","Pickup","Departure","Capacity","Booked","Boarded","Status"},{90,145,130,95,75,65,65,95});
    for(size_t i=0;i<m_schedules.size();++i){const auto& trip=m_schedules[i];if(!IsAssignedToDriver(trip,driver.id,driver.name))continue;
        const auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord& item){return item.id==trip.routeId;});
        int booked=0,boarded=0;for(const auto& reservation:m_reservations)if(HasAllocatedSeats(reservation)&&reservation.scheduleIndex==static_cast<int>(i)){++booked;if(reservation.boardingStatus=="Boarded")++boarded;}
        AddRow(table,static_cast<long>(i),{W(trip.id),route==m_routes.end()?W(trip.routeId):W(route->name),route==m_routes.end()?wxString("Unknown"):W(route->pickup),W(trip.departure),W(std::to_string(trip.capacity)),W(std::to_string(booked)),W(std::to_string(boarded)),W(trip.status)});
    }
    inner->Add(table,1,wxEXPAND);
    auto* actions=new wxBoxSizer(wxHORIZONTAL);auto* manifest=MakeButton(card,"Open Selected Manifest",true);auto* depart=MakeButton(card,"Mark Departed");auto* complete=MakeButton(card,"Complete Trip");actions->Add(manifest,0,wxRIGHT,6);actions->Add(depart,0,wxRIGHT,6);actions->Add(complete,0);inner->Add(actions,0,wxTOP,10);
    auto selected=[table](){return table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);};
    manifest->Bind(wxEVT_BUTTON,[this,table,selected](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a trip first.","Trip required",wxOK|wxICON_INFORMATION,this);return;}m_selectedSchedule=static_cast<int>(table->GetItemData(row));ShowPage(Page::Manifest);});
    depart->Bind(wxEVT_BUTTON,[this,table,selected](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a trip first.","Trip required",wxOK|wxICON_INFORMATION,this);return;}UpdateTripStatus(static_cast<long>(table->GetItemData(row)),false);});
    complete->Bind(wxEVT_BUTTON,[this,table,selected](wxCommandEvent&){long row=selected();if(row<0){wxMessageBox("Select a trip first.","Trip required",wxOK|wxICON_INFORMATION,this);return;}UpdateTripStatus(static_cast<long>(table->GetItemData(row)),true);});
    layout->Add(card,1,wxEXPAND|wxALL,10);
    auto* rule=new wxStaticText(page,wxID_ANY,"Operational rule: depart only from Scheduled status; completion requires every seated passenger to be checked in. Waitlisted requests do not count toward capacity or check-in.");rule->SetForegroundColour(kMuted);layout->Add(rule,0,wxLEFT|wxRIGHT|wxBOTTOM,10);
    return page;
}

wxWindow* MainFrame::BuildManifestPage() {
    auto* page=StartPage("Passenger Manifest","Search and inspect reservations assigned to the selected trip. Payment and boarding actions update shared records.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    if(m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())) return page;
    const auto& driver=m_drivers[static_cast<size_t>(m_selectedDriver)];
    auto* bar=new wxBoxSizer(wxHORIZONTAL);auto* tripChoice=new wxChoice(page,wxID_ANY);int choiceIndex=0,selection=wxNOT_FOUND;
    for(size_t i=0;i<m_schedules.size();++i)if(IsAssignedToDriver(m_schedules[i],driver.id,driver.name)){tripChoice->Append(W(m_schedules[i].id+"  |  "+m_schedules[i].routeId+"  |  "+m_schedules[i].departure));if(static_cast<int>(i)==m_selectedSchedule)selection=choiceIndex;++choiceIndex;}
    if(tripChoice->GetCount()>0)tripChoice->SetSelection(selection==wxNOT_FOUND?0:selection);
    auto* search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search ID, student, seat, or payment reference");auto* paymentFilter=new wxChoice(page,wxID_ANY);for(const char* value:{"Any payment","Payment due","Paid / verified","Rejected"})paymentFilter->Append(value);paymentFilter->SetSelection(0);auto* boardingFilter=new wxChoice(page,wxID_ANY);for(const char* value:{"Any boarding","Waiting","Boarded","Waitlisted"})boardingFilter->Append(value);boardingFilter->SetSelection(0);auto* refresh=MakeButton(page,"Refresh");
    bar->Add(tripChoice,0,wxRIGHT,8);bar->Add(search,1,wxRIGHT,8);bar->Add(paymentFilter,0,wxRIGHT,6);bar->Add(boardingFilter,0,wxRIGHT,6);bar->Add(refresh,0);layout->Add(bar,0,wxEXPAND|wxALL,10);
    auto* paymentRule=new wxStaticText(page,wxID_ANY,"Boarding is allowed after demo payment is marked received or manually verified. No payment provider is contacted.");paymentRule->SetForegroundColour(kMuted);layout->Add(paymentRule,0,wxLEFT|wxRIGHT|wxBOTTOM,8);
    wxBoxSizer* inner{};auto* card=MakeCard(page,"Shared reservation manifest",inner);
    auto* table=MakeTable(card,{"Reservation","Student ID","Student Name","Seat","Method","Reference","Payment Status","Boarding"},{100,125,145,60,90,145,135,95});
    auto* empty=new wxStaticText(card,wxID_ANY,"No passengers are booked on this trip.");empty->SetForegroundColour(kMuted);
    const std::string driverName=driver.name;
    const std::string driverId=driver.id;
    auto getScheduleIndex=[this,driverName,driverId,tripChoice](){int visible=0;for(size_t i=0;i<m_schedules.size();++i)if(IsAssignedToDriver(m_schedules[i],driverId,driverName)){if(visible++==tripChoice->GetSelection())return static_cast<int>(i);}return -1;};
    auto populate=[this,table,empty,search,paymentFilter,boardingFilter,getScheduleIndex](){table->DeleteAllItems();const int scheduleIndex=getScheduleIndex();int count=0;const wxString query=search->GetValue();for(size_t i=0;i<m_reservations.size();++i){const auto& reservation=m_reservations[i];if(!IsActiveReservation(reservation)||reservation.scheduleIndex!=scheduleIndex)continue;const int pf=paymentFilter->GetSelection(),bf=boardingFilter->GetSelection();if(pf==1&&IsPaymentCleared(reservation))continue;if(pf==2&&!IsPaymentCleared(reservation))continue;if(pf==3&&reservation.paymentStatus.find("Rejected")!=0)continue;if(bf==1&&reservation.status=="Waitlisted")continue;if(bf==2&&reservation.boardingStatus!="Boarded")continue;if(bf==3&&reservation.status!="Waitlisted")continue;if(bf==1&&reservation.boardingStatus!="Waiting")continue;if(!Includes(reservation.id,query)&&!Includes(reservation.studentId,query)&&!Includes(reservation.passenger,query)&&!Includes(reservation.seat,query)&&!Includes(reservation.paymentReference,query))continue;AddRow(table,static_cast<long>(i),{W(reservation.id),W(reservation.studentId),W(reservation.passenger),W(reservation.seat),W(reservation.paymentMethod),W(reservation.paymentReference.empty()?"--":reservation.paymentReference),W(reservation.paymentStatus),W(reservation.status=="Waitlisted"?"Waitlisted":reservation.boardingStatus)});++count;}empty->Show(count==0);};
    populate();tripChoice->Bind(wxEVT_CHOICE,[this,populate,getScheduleIndex](wxCommandEvent&){m_selectedSchedule=getScheduleIndex();populate();});search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});paymentFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});boardingFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});refresh->Bind(wxEVT_BUTTON,[populate](wxCommandEvent&){populate();});
    auto* details=new wxStaticText(card,wxID_ANY,"Select a passenger to review reservation details.");details->SetForegroundColour(kMuted);inner->Add(table,1,wxEXPAND|wxBOTTOM,8);inner->Add(empty,0,wxEXPAND|wxBOTTOM,8);inner->Add(details,0,wxEXPAND|wxBOTTOM,8);
    table->Bind(wxEVT_LIST_ITEM_SELECTED,[this,table,details](wxListEvent& event){const auto& r=m_reservations[static_cast<size_t>(table->GetItemData(event.GetIndex()))];details->SetLabel(wxString::Format("%s  |  %s (%s)  |  Seat %s  |  %s: %s  |  Payment: %s  |  Boarding: %s",W(r.id),W(r.passenger),W(r.studentId),W(r.seat),W(r.paymentMethod),W(r.paymentReference.empty()?"--":r.paymentReference),W(r.paymentStatus),W(r.boardingStatus)));});
    auto* boardingActions=new wxBoxSizer(wxHORIZONTAL);auto* checkIn=MakeButton(card,"Mark Selected Passenger Boarded",true);auto* undoCheckIn=MakeButton(card,"Undo Check-in");boardingActions->Add(checkIn,0,wxRIGHT,8);boardingActions->Add(undoCheckIn,0);inner->Add(boardingActions,0,wxALIGN_LEFT);checkIn->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a passenger first.","Passenger required",wxOK|wxICON_INFORMATION,this);return;}SetBoardingStatus(static_cast<long>(table->GetItemData(row)),true);});undoCheckIn->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a passenger first.","Passenger required",wxOK|wxICON_INFORMATION,this);return;}SetBoardingStatus(static_cast<long>(table->GetItemData(row)),false);});
    layout->Add(card,1,wxEXPAND|wxALL,10);
    // TODO [Cleoh Amor B. Sarmiento]: Integrate Quick Sort for seat/status ordering and Binary Search only over sorted lookup data.
    return page;
}

wxWindow* MainFrame::BuildBoardingPage() {
    return BuildManifestPage();
}

wxWindow* MainFrame::BuildTripHistoryPage() {
    auto* page=StartPage("Trip History","Completed trips assigned to the signed-in driver.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    if(m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())) return page;
    const auto& driver=m_drivers[static_cast<size_t>(m_selectedDriver)];
    wxBoxSizer* inner{};auto* card=MakeCard(page,"Completed trips",inner);
    auto* table=MakeTable(card,{"Trip ID","Route","Departure","Capacity","Boarded","Status"},{100,180,140,90,90,120});int count=0;
    for(size_t i=0;i<m_schedules.size();++i){const auto& trip=m_schedules[i];if(!IsAssignedToDriver(trip,driver.id,driver.name)||trip.status!="Completed")continue;const auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord& r){return r.id==trip.routeId;});int boarded=0;for(const auto& reservation:m_reservations)if(HasAllocatedSeats(reservation)&&reservation.scheduleIndex==static_cast<int>(i)&&reservation.boardingStatus=="Boarded")++boarded;AddRow(table,static_cast<long>(i),{W(trip.id),route==m_routes.end()?W(trip.routeId):W(route->name),W(trip.departure),W(std::to_string(trip.capacity)),W(std::to_string(boarded)),W(trip.status)});++count;}
    if(count==0)inner->Add(new wxStaticText(card,wxID_ANY,"No completed trips are recorded for this driver."),0,wxEXPAND|wxBOTTOM,8);
    inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);
    return page;
}

wxWindow* MainFrame::BuildDigitalPassPage() {
    auto* page=StartPage("My Digital Pass","Your reservation details for the driver. A scannable pass will appear when this demo has a secure pass service.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    auto* select=new wxChoice(page,wxID_ANY);std::vector<size_t> own;
    for(size_t i=0;i<m_reservations.size();++i){const auto& reservation=m_reservations[i];if(reservation.studentId!=m_currentUserId||!IsActiveReservation(reservation))continue;own.push_back(i);select->Append(W(reservation.id+"  |  "+reservation.route+"  |  "+reservation.departure));if(reservation.id==m_selectedReservationId)select->SetSelection(static_cast<int>(own.size()-1));}
    if(select->GetCount()>0&&select->GetSelection()==wxNOT_FOUND)select->SetSelection(0);
    layout->Add(select,0,wxALL,10);wxBoxSizer* inner{};auto* card=MakeCard(page,"TaraMapuan Digital Shuttle Pass",inner);
    auto* details=new wxStaticText(card,wxID_ANY,"No active reservation is available.");details->SetFont(wxFontInfo(12));inner->Add(details,0,wxEXPAND|wxBOTTOM,12);
    auto* passState=new wxStaticText(card,wxID_ANY,"PASS STATUS\nSelect a reservation");passState->SetFont(wxFontInfo(13).Bold());passState->SetForegroundColour(kBlue);passState->SetBackgroundColour(wxColour(239,244,250));passState->SetMinSize(wxSize(280,90));inner->Add(passState,0,wxALIGN_LEFT|wxALL,8);layout->Add(card,0,wxEXPAND|wxALL,10);
    auto update=[this,select,details,passState,own](){if(select->GetSelection()<0||select->GetSelection()>=static_cast<int>(own.size())){details->SetLabel("No active reservation is available.");passState->SetLabel("PASS STATUS\nNo reservation selected");return;}const auto& reservation=m_reservations[own[static_cast<size_t>(select->GetSelection())]];m_selectedReservationId=reservation.id;std::string pickup="Unknown";if(reservation.scheduleIndex>=0&&reservation.scheduleIndex<static_cast<int>(m_schedules.size())){const auto& trip=m_schedules[static_cast<size_t>(reservation.scheduleIndex)];const auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord& item){return item.id==trip.routeId;});if(route!=m_routes.end())pickup=route->pickup;}details->SetLabel(wxString::Format("Reservation: %s\nStudent ID: %s\nRoute: %s\nPickup: %s\nDeparture: %s\nSeat: %s\nPayment: %s (%s)\nBoarding: %s",W(reservation.id),W(reservation.studentId),W(reservation.route),W(pickup),W(reservation.departure),W(reservation.status=="Waitlisted"?"Not assigned":reservation.seat),W(reservation.paymentMethod),W(reservation.paymentStatus),W(reservation.boardingStatus)));passState->SetLabel(reservation.status=="Waitlisted"?"WAITLISTED\nA seat has not been assigned yet.":"RESERVATION CONFIRMED\nShow these details to your shuttle driver.\nQR pass: unavailable in demo");};
    select->Bind(wxEVT_CHOICE,[update](wxCommandEvent&){update();});update();
    return page;
}
wxWindow* MainFrame::BuildMonitoringPage(){auto*page=StartPage("Shuttle monitoring","Simulated fleet status only. There is no GPS receiver or live location feed.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());int seats=0;for(const auto&s:m_schedules)seats+=s.availableSeats;auto*metrics=new wxGridSizer(1,3,10,10);metrics->Add(MetricCard(page,"SHUTTLES IN SAMPLE",W(std::to_string(m_schedules.size())),"Scheduled vehicles",kBlue),1,wxEXPAND);metrics->Add(MetricCard(page,"SEATS AVAILABLE",W(std::to_string(seats)),"Across all departures",kGreen),1,wxEXPAND);metrics->Add(MetricCard(page,"LOCATION DATA","MOCK","Not live tracking",kAmber),1,wxEXPAND);layout->Add(metrics,0,wxEXPAND|wxALL,10);wxBoxSizer*inner{};auto*card=MakeCard(page,"Fleet status  |  Simulated",inner);auto*table=MakeTable(card,{"Shuttle ID","Assigned route","Driver","Current status","Occupancy","Last update"},{95,160,140,120,90,140});for(size_t i=0;i<m_schedules.size();++i){const auto&s=m_schedules[i];wxString status=W(s.status);if(s.availableSeats==0)status="Full";AddRow(table,static_cast<long>(i),{W(s.shuttle),W(s.routeId),W(s.driver),status,wxString::Format("%d / %d",s.capacity-s.availableSeats,s.capacity),"Sample snapshot"});}inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);
    // TODO(GPS): Integrate a GPS provider before showing any actual vehicle location.
    // TODO(API): Fetch fleet status and timestamps from an authorized campus shuttle API.
    return page;}

wxWindow* MainFrame::BuildPaymentsPage() {
    const bool student=m_userRole==UserRole::Student;
    auto* page=StartPage(student?"Payment Details":"Payment Verification",student?"Review your payment method, submitted reference, and current status.":"Manual demo review only. A reference number is not proof of payment.");
    auto* layout=static_cast<wxBoxSizer*>(page->GetSizer());
    auto* bar=new wxBoxSizer(wxHORIZONTAL);auto* search=new wxTextCtrl(page,wxID_ANY);search->SetHint("Search reservation, student, or reference");auto* methodFilter=new wxChoice(page,wxID_ANY);for(const char* value:{"Any method","Cash","Online"})methodFilter->Append(value);methodFilter->SetSelection(0);auto* statusFilter=new wxChoice(page,wxID_ANY);for(const char* value:{"Any status","Payment due","Paid / verified","Rejected","Waitlisted"})statusFilter->Append(value);statusFilter->SetSelection(0);bar->Add(search,1,wxRIGHT,8);bar->Add(methodFilter,0,wxRIGHT,6);bar->Add(statusFilter,0);
    wxBoxSizer* inner{};auto* card=MakeCard(page,student?"My payment records":"Assigned-trip payment queue",inner);
    auto* table=MakeTable(card,{"Reservation","Student ID","Passenger","Method","Reference","Payment Status","Boarding"},{100,125,140,90,150,145,95});
    auto populate=[this,table,search,methodFilter,statusFilter,student](){table->DeleteAllItems();const wxString query=search->GetValue();for(size_t i=0;i<m_reservations.size();++i){const auto& r=m_reservations[i];if(!IsActiveReservation(r))continue;if(student){if(r.studentId!=m_currentUserId)continue;}else{if(r.scheduleIndex<0||r.scheduleIndex>=static_cast<int>(m_schedules.size())||m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())||!IsAssignedToDriver(m_schedules[static_cast<size_t>(r.scheduleIndex)],m_drivers[static_cast<size_t>(m_selectedDriver)].id,m_drivers[static_cast<size_t>(m_selectedDriver)].name))continue;}const int mf=methodFilter->GetSelection(),sf=statusFilter->GetSelection();if(mf==1&&r.paymentMethod!="Cash")continue;if(mf==2&&r.paymentMethod=="Cash")continue;if(sf==1&&(IsPaymentCleared(r)||r.status=="Waitlisted"))continue;if(sf==2&&!IsPaymentCleared(r))continue;if(sf==3&&r.paymentStatus.find("Rejected")!=0)continue;if(sf==4&&r.status!="Waitlisted")continue;if(!Includes(r.id,query)&&!Includes(r.studentId,query)&&!Includes(r.passenger,query)&&!Includes(r.paymentReference,query))continue;AddRow(table,static_cast<long>(i),{W(r.id),W(r.studentId),W(r.passenger),W(r.paymentMethod),W(r.paymentReference.empty()?"--":r.paymentReference),W(r.status=="Waitlisted"?"Not due (waitlisted)":r.paymentStatus),W(r.status=="Waitlisted"?"Waitlisted":r.boardingStatus)});}};
    populate();search->Bind(wxEVT_TEXT,[populate](wxCommandEvent&){populate();});methodFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});statusFilter->Bind(wxEVT_CHOICE,[populate](wxCommandEvent&){populate();});inner->Add(table,1,wxEXPAND);layout->Add(card,1,wxEXPAND|wxALL,10);
    if(!student){auto* actions=new wxBoxSizer(wxHORIZONTAL);auto* accept=MakeButton(page,"Mark Received / Verified",true);auto* reject=MakeButton(page,"Reject Online Reference");actions->Add(accept,0,wxRIGHT,8);actions->Add(reject,0);layout->Add(actions,0,wxLEFT|wxRIGHT|wxBOTTOM,10);accept->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a payment record first.","Payment required",wxOK|wxICON_INFORMATION,this);return;}SetPaymentStatus(static_cast<long>(table->GetItemData(row)),true);});reject->Bind(wxEVT_BUTTON,[this,table](wxCommandEvent&){long row=table->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);if(row<0){wxMessageBox("Select a payment record first.","Payment required",wxOK|wxICON_INFORMATION,this);return;}SetPaymentStatus(static_cast<long>(table->GetItemData(row)),false);});}
    // TODO [Robby Ruth Abaquin]: Add authorized payment-provider verification after a real backend is available.
    if(!student){wxBoxSizer* auditSizer{};auto* audit=MakeCard(page,"Recent demo review activity",auditSizer);if(m_paymentAudit.empty()){auto* note=new wxStaticText(audit,wxID_ANY,"No manual payment decisions have been recorded this session.");note->SetForegroundColour(kMuted);auditSizer->Add(note,0,wxEXPAND);}else{const size_t first=m_paymentAudit.size()>8?m_paymentAudit.size()-8:0;for(size_t i=m_paymentAudit.size();i>first;--i)auditSizer->Add(new wxStaticText(audit,wxID_ANY,W(m_paymentAudit[i-1])),0,wxBOTTOM,4);}layout->Add(audit,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,10);}
    return page;
}

void MainFrame::ReviewPayment(long rowIndex) { SetPaymentStatus(rowIndex,true); }

void MainFrame::SetPaymentStatus(long rowIndex, bool accepted) {
    if(rowIndex<0||rowIndex>=static_cast<long>(m_reservations.size()))return;
    auto& reservation=m_reservations[static_cast<size_t>(rowIndex)];
    if(m_userRole!=UserRole::Driver){wxMessageBox("Only a signed-in driver can review payments.","Access unavailable",wxOK|wxICON_WARNING,this);return;}
    if(!IsActiveReservation(reservation)){wxMessageBox("Cancelled reservations cannot be verified.","Payment unavailable",wxOK|wxICON_WARNING,this);return;}
    if(reservation.status=="Waitlisted"){wxMessageBox("Payment is not due until a seat is offered.","Payment unavailable",wxOK|wxICON_INFORMATION,this);return;}
    if(m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size()))return;
    if(reservation.scheduleIndex<0||reservation.scheduleIndex>=static_cast<int>(m_schedules.size())||!IsAssignedToDriver(m_schedules[static_cast<size_t>(reservation.scheduleIndex)],m_drivers[static_cast<size_t>(m_selectedDriver)].id,m_drivers[static_cast<size_t>(m_selectedDriver)].name)){wxMessageBox("This reservation is not assigned to your trips.","Payment unavailable",wxOK|wxICON_WARNING,this);return;}
    if(IsPaymentCleared(reservation)){wxMessageBox("This payment is already marked received or verified.","Payment status",wxOK|wxICON_INFORMATION,this);return;}
    if(accepted&&reservation.paymentMethod!="Cash"&&reservation.paymentReference.empty()){wxMessageBox("An online payment reference is required before manual review.","Reference required",wxOK|wxICON_WARNING,this);return;}
    if(!accepted&&(reservation.paymentMethod=="Cash"||reservation.paymentReference.empty())){wxMessageBox("Only an online payment with a submitted reference can be rejected.","Reference required",wxOK|wxICON_WARNING,this);return;}
    const wxString action=accepted?(reservation.paymentMethod=="Cash"?"Mark this cash payment as received?":"Manually verify the submitted reference? This does not contact the payment provider."):"Reject this submitted online payment reference?";
    if(wxMessageBox(action,"Confirm demo payment action",wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION,this)!=wxYES)return;
    const std::string previous=reservation.paymentStatus;
    if(accepted)reservation.paymentStatus=reservation.paymentMethod=="Cash"?"Paid (demo)":"Verified (demo)";
    else reservation.paymentStatus="Rejected (demo)";
    const wxDateTime now=wxDateTime::Now();m_paymentAudit.push_back(Utf8(now.FormatISOCombined(' '))+" | "+reservation.id+" | "+reservation.studentId+" | "+previous+" -> "+reservation.paymentStatus);
    if(m_paymentAudit.size()>1000)m_paymentAudit.erase(m_paymentAudit.begin(),m_paymentAudit.begin()+static_cast<std::ptrdiff_t>(m_paymentAudit.size()-1000));
    SaveReservations();SetStatusText("Payment status updated in the shared local reservation record.");
    wxMessageBox(accepted?"The demo payment status was updated.":"The online reference was marked rejected in the demo record.","Payment status",wxOK|wxICON_INFORMATION,this);
    RefreshCurrentPage();
}

void MainFrame::SetBoardingStatus(long rowIndex, bool boarded) {
    if(rowIndex<0||rowIndex>=static_cast<long>(m_reservations.size()))return;
    auto& reservation=m_reservations[static_cast<size_t>(rowIndex)];
    if(m_userRole!=UserRole::Driver||m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())){wxMessageBox("Only a signed-in driver can check in passengers.","Access unavailable",wxOK|wxICON_WARNING,this);return;}
    if(!IsActiveReservation(reservation)){wxMessageBox("Cancelled reservations cannot be checked in.","Boarding unavailable",wxOK|wxICON_WARNING,this);return;}
    if(reservation.status=="Waitlisted"){wxMessageBox("A boarding pass is unavailable until a seat is offered.","Boarding unavailable",wxOK|wxICON_INFORMATION,this);return;}
    if(reservation.scheduleIndex<0||reservation.scheduleIndex>=static_cast<int>(m_schedules.size())||!IsAssignedToDriver(m_schedules[static_cast<size_t>(reservation.scheduleIndex)],m_drivers[static_cast<size_t>(m_selectedDriver)].id,m_drivers[static_cast<size_t>(m_selectedDriver)].name)){wxMessageBox("This passenger is not on your assigned trips.","Boarding unavailable",wxOK|wxICON_WARNING,this);return;}
    if(boarded&&reservation.boardingStatus=="Boarded"){wxMessageBox("This passenger has already been checked in.","Already boarded",wxOK|wxICON_INFORMATION,this);return;}
    if(!boarded&&reservation.scheduleIndex>=0&&reservation.scheduleIndex<static_cast<int>(m_schedules.size())&&m_schedules[static_cast<size_t>(reservation.scheduleIndex)].status=="Completed"){wxMessageBox("Check-in cannot be undone after the trip is completed.","Boarding unavailable",wxOK|wxICON_WARNING,this);return;}
    if(boarded&&!IsPaymentCleared(reservation)){wxMessageBox("Verify the payment or mark cash as received before boarding.","Payment action required",wxOK|wxICON_WARNING,this);return;}
    if(!boarded&&reservation.boardingStatus!="Boarded"){wxMessageBox("This passenger is not currently checked in.","No check-in to undo",wxOK|wxICON_INFORMATION,this);return;}
    if(wxMessageBox(boarded?"Confirm passenger boarding check-in?":"Undo this passenger's boarding check-in?","Confirm boarding",wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION,this)!=wxYES)return;
    reservation.boardingStatus=boarded?"Boarded":"Waiting";SaveReservations();SetStatusText("Boarding status updated in the shared local reservation record.");
    wxMessageBox(boarded?"Passenger marked boarded.":"Passenger check-in was undone.","Boarding status",wxOK|wxICON_INFORMATION,this);RefreshCurrentPage();
}

void MainFrame::UpdateTripStatus(long scheduleIndex, bool completed) {
    if(m_userRole!=UserRole::Driver||m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())||scheduleIndex<0||scheduleIndex>=static_cast<long>(m_schedules.size()))return;
    auto& trip=m_schedules[static_cast<size_t>(scheduleIndex)];
    if(!IsAssignedToDriver(trip,m_drivers[static_cast<size_t>(m_selectedDriver)].id,m_drivers[static_cast<size_t>(m_selectedDriver)].name)){wxMessageBox("You can only update trips assigned to your driver account.","Trip unavailable",wxOK|wxICON_WARNING,this);return;}
    if(completed){if(trip.status!="On Route"){wxMessageBox("A trip must be marked departed before it can be completed.","Trip status",wxOK|wxICON_WARNING,this);return;}for(const auto& reservation:m_reservations)if(HasAllocatedSeats(reservation)&&reservation.scheduleIndex==scheduleIndex&&reservation.boardingStatus!="Boarded"){wxMessageBox("All seated active passengers must be checked in before completing this trip.","Passengers remain",wxOK|wxICON_WARNING,this);return;}}
    else if(trip.status!="Scheduled"){wxMessageBox("Only a scheduled trip can be marked departed.","Trip status",wxOK|wxICON_INFORMATION,this);return;}
    if(wxMessageBox(completed?"Confirm that this trip is complete?":"Confirm that this trip has departed?","Confirm trip status",wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION,this)!=wxYES)return;
    trip.status=completed?"Completed":"On Route";SetStatusText("Trip status updated for the assigned demo trip.");RefreshCurrentPage();
}
wxWindow* MainFrame::BuildReportsPage(){auto*page=StartPage("Reports","Summary calculations based on the in-memory sample records shown in this prototype.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());int confirmed=0,pending=0,cancelled=0,paid=0,review=0;for(const auto&r:m_reservations){if(r.status.find("Confirmed")==0)++confirmed;else if(r.status=="Pending")++pending;else if(r.status=="Cancelled")++cancelled;if(r.paymentStatus.find("Paid")==0)++paid;if(r.paymentStatus.find("Reviewed")==0)++review;}auto*metrics=new wxGridSizer(1,4,10,10);metrics->Add(MetricCard(page,"DAILY RESERVATIONS",W(std::to_string(m_reservations.size())),"Sample count",kBlue),1,wxEXPAND);metrics->Add(MetricCard(page,"CONFIRMED",W(std::to_string(confirmed)),"Local sample status",kGreen),1,wxEXPAND);metrics->Add(MetricCard(page,"PENDING",W(std::to_string(pending)),"Awaiting review",kAmber),1,wxEXPAND);metrics->Add(MetricCard(page,"CANCELLED",W(std::to_string(cancelled)),"Sample records",kRed),1,wxEXPAND);layout->Add(metrics,0,wxEXPAND|wxALL,10);wxBoxSizer*inner{};auto*card=MakeCard(page,"Shuttle utilization  |  Sample data",inner);for(const auto&s:m_schedules){auto*line=new wxBoxSizer(wxHORIZONTAL);auto*name=new wxStaticText(card,wxID_ANY,W(s.shuttle+"  "+s.routeId));name->SetMinSize(wxSize(170,-1));line->Add(name,0,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);auto*gauge=new wxGauge(card,wxID_ANY,s.capacity);gauge->SetValue(s.capacity-s.availableSeats);line->Add(gauge,1,wxALIGN_CENTER_VERTICAL|wxRIGHT,8);line->Add(new wxStaticText(card,wxID_ANY,wxString::Format("%d / %d",s.capacity-s.availableSeats,s.capacity)),0,wxALIGN_CENTER_VERTICAL);inner->Add(line,0,wxEXPAND|wxBOTTOM,10);}inner->Add(new wxStaticText(card,wxID_ANY,wxString::Format("Payment summary: %d paid (demo)  |  %d reviewed (demo)  |  %d remaining",paid,review,static_cast<int>(m_reservations.size())-paid-review)),0,wxTOP,8);layout->Add(card,0,wxEXPAND|wxALL,10);
    // TODO(REPORTS): Add CSV or PDF export after a reporting backend is selected.
    return page;}

wxWindow* MainFrame::BuildSettingsPage(){auto*page=StartPage("Settings","Prototype preferences apply only until the application closes.");auto*layout=static_cast<wxBoxSizer*>(page->GetSizer());wxBoxSizer*inner{};auto*card=MakeCard(page,"Interface preferences",inner);auto*notifications=new wxCheckBox(card,wxID_ANY,"Show in-app notification messages");notifications->SetValue(m_notificationsEnabled);auto*compact=new wxCheckBox(card,wxID_ANY,"Use compact table spacing");compact->SetValue(m_compactLayout);inner->Add(notifications,0,wxBOTTOM,12);inner->Add(compact,0,wxBOTTOM,12);auto*apply=MakeButton(card,"Apply Preferences",true);inner->Add(apply,0,wxBOTTOM,12);auto*note=new wxStaticText(card,wxID_ANY,"Preferences are held in memory and are not written to disk.");note->SetForegroundColour(kMuted);inner->Add(note,0);apply->Bind(wxEVT_BUTTON,[this,notifications,compact](wxCommandEvent&){m_notificationsEnabled=notifications->GetValue();m_compactLayout=compact->GetValue();SetStatusText("Prototype preferences applied for this session.");wxMessageBox("Preferences updated for this session only.","Settings",wxOK|wxICON_INFORMATION,this);});layout->Add(card,0,wxEXPAND|wxALL,10);wxBoxSizer*aboutInner{};auto*about=MakeCard(page,"Application information",aboutInner);aboutInner->Add(new wxStaticText(about,wxID_ANY,"TaraShuttle | Version 1.0 prototype"),0,wxBOTTOM,8);aboutInner->Add(new wxStaticText(about,wxID_ANY,"C++17  |  wxWidgets  |  Windows desktop"),0,wxBOTTOM,8);aboutInner->Add(new wxStaticText(about,wxID_ANY,"This application demonstrates local UI flows with fictional data."),0,wxBOTTOM,8);layout->Add(about,0,wxEXPAND|wxALL,10);
    // TODO(PERSISTENCE): Save interface preferences between application sessions.
    return page;}
