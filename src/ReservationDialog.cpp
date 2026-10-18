#include "ReservationDialog.h"

#include <wx/datetime.h>

namespace {
bool IsBookable(const ScheduleRecord& schedule) {
    return schedule.availableSeats > 0 && schedule.status != "Full" &&
           schedule.status != "On Route" && schedule.status != "Completed";
}
}

ReservationDialog::ReservationDialog(wxWindow* parent,
                                     const std::vector<RouteRecord>& routes,
                                     const std::vector<ScheduleRecord>& schedules,
                                     const std::string& presetName,
                                     const std::string& presetStudentId)
    : wxDialog(parent, wxID_ANY, "New Reservation", wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_routes(routes), m_schedules(schedules) {
    SetMinSize(wxSize(540, 580));
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* intro = new wxStaticText(this, wxID_ANY,
        "Create a local sample reservation. No payment or booking is sent to a service.");
    intro->Wrap(480);
    root->Add(intro, 0, wxEXPAND | wxALL, 16);

    auto* form = new wxFlexGridSizer(2, 9, 10);
    form->AddGrowableCol(1, 1);
    m_passenger = new wxTextCtrl(this, wxID_ANY);
    m_studentId = new wxTextCtrl(this, wxID_ANY);
    if (!presetName.empty()) m_passenger->SetValue(wxString::FromUTF8(presetName));
    if (!presetStudentId.empty()) {
        m_studentId->SetValue(wxString::FromUTF8(presetStudentId));
        m_studentId->Enable(false);
    }
    m_date = new wxTextCtrl(this, wxID_ANY, wxDateTime::Today().FormatISODate());
    m_route = new wxChoice(this, wxID_ANY);
    m_schedule = new wxChoice(this, wxID_ANY);
    m_seats = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 1, 18, 1);
    m_payment = new wxChoice(this, wxID_ANY);
    m_payment->Append("Cash");
    m_payment->Append("GCash");
    m_payment->Append("Maya");
    m_payment->Append("GoTyme");
    m_payment->SetSelection(0);
    m_paymentReference = new wxTextCtrl(this, wxID_ANY);
    for (const auto& route : m_routes)
        m_route->Append(wxString::FromUTF8(route.id + " - " + route.name));
    if (!m_routes.empty()) m_route->SetSelection(0);

    auto addField = [form](const wxString& label, wxWindow* control) {
        form->Add(new wxStaticText(control->GetParent(), wxID_ANY, label), 0,
                  wxALIGN_CENTER_VERTICAL);
        form->Add(control, 1, wxEXPAND);
    };
    addField("Passenger name *", m_passenger);
    addField("Student ID *", m_studentId);
    addField("Travel date (YYYY-MM-DD) *", m_date);
    addField("Route *", m_route);
    addField("Departure schedule *", m_schedule);
    addField("Number of seats *", m_seats);
    addField("Payment method *", m_payment);
    addField("GCash reference", m_paymentReference);
    root->Add(form, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 18);

    m_summary = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_summary->SetForegroundColour(wxColour(21, 101, 192));
    root->Add(m_summary, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 18);

    auto* buttons = new wxStdDialogButtonSizer();
    auto* confirm = new wxButton(this, wxID_OK, "Create Sample Reservation");
    confirm->SetDefault();
    buttons->AddButton(confirm);
    buttons->AddButton(new wxButton(this, wxID_CANCEL));
    buttons->Realize();
    root->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);
    SetSizerAndFit(root);

    m_route->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateScheduleChoices(); });
    m_schedule->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateSummary(); });
    m_seats->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) { UpdateSummary(); });
    m_payment->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        m_paymentReference->Enable(m_payment->GetSelection() > 0);
    });
    Bind(wxEVT_BUTTON, &ReservationDialog::OnSubmit, this, wxID_OK);
    m_paymentReference->Enable(m_payment->GetSelection() > 0);
    UpdateScheduleChoices();
}

void ReservationDialog::UpdateScheduleChoices() {
    const int routeIndex = m_route->GetSelection();
    m_schedule->Clear();
    for (const auto& schedule : m_schedules) {
        if ((routeIndex < 0 || schedule.routeId != m_routes[static_cast<size_t>(routeIndex)].id) || !IsBookable(schedule))
            continue;
        m_schedule->Append(wxString::FromUTF8(schedule.id + "  |  " + schedule.departure +
                                              "  |  " + schedule.status));
    }
    if (m_schedule->GetCount() > 0) m_schedule->SetSelection(0);
    UpdateSummary();
}

void ReservationDialog::UpdateSummary() {
    const int routeIndex = m_route->GetSelection();
    int scheduleIndex = -1;
    if (routeIndex >= 0) {
        int visibleIndex = 0;
        for (size_t i = 0; i < m_schedules.size(); ++i) {
            if (m_schedules[i].routeId != m_routes[static_cast<size_t>(routeIndex)].id || !IsBookable(m_schedules[i])) continue;
            if (visibleIndex++ == m_schedule->GetSelection()) {
                scheduleIndex = static_cast<int>(i);
                break;
            }
        }
    }
    if (scheduleIndex < 0) {
        m_summary->SetLabel("No departure is available for this route.");
        return;
    }
    const auto& schedule = m_schedules[static_cast<size_t>(scheduleIndex)];
    m_summary->SetLabel(wxString::Format("Summary: %s | %s | %s | %d seat(s) available",
        wxString::FromUTF8(schedule.routeId), wxString::FromUTF8(schedule.departure),
        wxString::FromUTF8(schedule.shuttle), schedule.availableSeats));
}

ReservationDraft ReservationDialog::GetDraft() const {
    ReservationDraft draft;
    draft.passenger = m_passenger->GetValue().Strip(wxString::both);
    draft.studentId = m_studentId->GetValue().Strip(wxString::both);
    draft.travelDate = m_date->GetValue().Strip(wxString::both);
    draft.paymentMethod = m_payment->GetStringSelection();
    draft.paymentReference = m_paymentReference->GetValue().Strip(wxString::both);
    draft.routeIndex = m_route->GetSelection();
    draft.scheduleIndex = -1;
    const int visibleSelection = m_schedule->GetSelection();
    int visibleIndex = 0;
    if (draft.routeIndex >= 0) {
        for (size_t i = 0; i < m_schedules.size(); ++i) {
            if (m_schedules[i].routeId != m_routes[static_cast<size_t>(draft.routeIndex)].id || !IsBookable(m_schedules[i])) continue;
            if (visibleIndex++ == visibleSelection) {
                draft.scheduleIndex = static_cast<int>(i);
                break;
            }
        }
    }
    draft.seatCount = m_seats->GetValue();
    return draft;
}

void ReservationDialog::OnSubmit(wxCommandEvent&) {
    const auto draft = GetDraft();
    wxDateTime date;
    if (draft.passenger.empty() || draft.studentId.empty() || draft.travelDate.empty() ||
        draft.routeIndex < 0 || draft.scheduleIndex < 0) {
        wxMessageBox("Complete the required fields and select a route with a departure.",
                     "Reservation details required", wxOK | wxICON_WARNING, this);
        return;
    }
    if (!date.ParseISODate(draft.travelDate)) {
        wxMessageBox("Enter the travel date in YYYY-MM-DD format.", "Check travel date",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    if (draft.paymentMethod != "Cash" && draft.paymentReference.empty()) {
        wxMessageBox("Enter the online payment reference, or choose Cash.",
                     "Payment reference required", wxOK | wxICON_WARNING, this);
        return;
    }
    EndModal(wxID_OK);
    // TODO [Justin Axcel C. Magpily]: Continue student reservation UI work, payment-method validation, and digital-pass presentation.
}
