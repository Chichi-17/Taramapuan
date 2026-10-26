#pragma once

#include "Models.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>

struct ReservationDraft {
    wxString passenger;
    wxString studentId;
    wxString travelDate;
    wxString paymentMethod;
    wxString paymentReference;
    int routeIndex{-1};
    int scheduleIndex{-1};
    int seatCount{1};
    bool joinWaitlist{false};
};

class ReservationDialog final : public wxDialog {
public:
    ReservationDialog(wxWindow* parent,
                      const std::vector<RouteRecord>& routes,
                      const std::vector<ScheduleRecord>& schedules,
                      const std::string& presetName = {},
                      const std::string& presetStudentId = {});

    ReservationDraft GetDraft() const;

private:
    void UpdateScheduleChoices();
    void UpdateSummary();
    void OnSubmit(wxCommandEvent& event);

    const std::vector<RouteRecord>& m_routes;
    const std::vector<ScheduleRecord>& m_schedules;
    wxTextCtrl* m_passenger{};
    wxTextCtrl* m_studentId{};
    wxTextCtrl* m_date{};
    wxChoice* m_route{};
    wxChoice* m_schedule{};
    wxSpinCtrl* m_seats{};
    wxChoice* m_payment{};
    wxTextCtrl* m_paymentReference{};
    wxCheckBox* m_joinWaitlist{};
    wxStaticText* m_summary{};
};
