#pragma once

#include "Models.h"

#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/wx.h>

#include <utility>
#include <vector>

class MainFrame final : public wxFrame {
public:
    MainFrame();

private:
    enum class Page {
        Landing, RoleSelect, Login, Dashboard, StudentDashboard, Reserve, Reservations,
        DigitalPass, Routes, Schedules, Passengers, Drivers, DriverDashboard,
        AssignedTrips, Manifest, Boarding, TripHistory, Monitoring, Payments, Reports, Settings
    };
    enum class UserRole { None, Student, Driver };

    wxScrolledWindow* StartPage(const wxString& title, const wxString& subtitle);
    wxWindow* BuildPage(Page page);
    void ShowPage(Page page);
    void RefreshCurrentPage();
    void UpdateNavigationStyle();
    void RebuildNavigation();
    void CreateReservation();
    void LoadReservations();
    void SaveReservations() const;
    void CancelReservation(long rowIndex);
    void ReviewPayment(long rowIndex);
    void SetPaymentStatus(long rowIndex, bool accepted);
    void SetBoardingStatus(long rowIndex, bool boarded);
    void UpdateTripStatus(long scheduleIndex, bool completed);
    void AddOrEditRoute(long routeIndex = -1);
    void AddSchedule();
    void AddOrEditDriver(long driverIndex = -1);
    void ShowReservationDetails(long rowIndex);

    wxWindow* BuildDashboard();
    wxWindow* BuildLandingPage();
    wxWindow* BuildRoleSelectPage();
    wxWindow* BuildLoginPage();
    wxWindow* BuildStudentDashboard();
    wxWindow* BuildReservationPage();
    wxWindow* BuildReservationsPage();
    wxWindow* BuildRoutesPage();
    wxWindow* BuildSchedulesPage();
    wxWindow* BuildPassengersPage();
    wxWindow* BuildDriversPage();
    wxWindow* BuildDriverDashboard();
    wxWindow* BuildAssignedTripsPage();
    wxWindow* BuildManifestPage();
    wxWindow* BuildBoardingPage();
    wxWindow* BuildTripHistoryPage();
    wxWindow* BuildDigitalPassPage();
    wxWindow* BuildMonitoringPage();
    wxWindow* BuildPaymentsPage();
    wxWindow* BuildReportsPage();
    wxWindow* BuildSettingsPage();

    std::vector<RouteRecord> m_routes{SampleRoutes()};
    std::vector<ScheduleRecord> m_schedules{SampleSchedules()};
    std::vector<ReservationRecord> m_reservations{SampleReservations()};
    std::vector<PassengerRecord> m_passengers{SamplePassengers()};
    std::vector<DriverRecord> m_drivers{SampleDrivers()};
    wxPanel* m_pageHost{};
    wxPanel* m_sidebar{};
    wxScrolledWindow* m_navScroll{};
    wxBoxSizer* m_navSizer{};
    wxBoxSizer* m_pageSizer{};
    wxStaticText* m_pageTitle{};
    wxStaticText* m_pageSubtitle{};
    std::vector<std::pair<Page, wxButton*>> m_navigation;
    Page m_currentPage{Page::Dashboard};
    UserRole m_selectedRole{UserRole::None};
    UserRole m_userRole{UserRole::None};
    std::string m_currentUserId;
    std::string m_selectedReservationId;
    int m_selectedSchedule{-1};
    int m_selectedDriver{0};
    int m_reservationSequence{24100};
    bool m_notificationsEnabled{true};
    bool m_compactLayout{false};
    wxString m_departureStatus{"Scheduled"};
};
