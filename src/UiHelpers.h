#pragma once
#include "Models.h"
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {
const wxColour kBlue(21, 101, 192);
const wxColour kNavy(20, 42, 74);
const wxColour kPrimaryRed(214, 40, 40);
const wxColour kRed(198, 40, 40);
const wxColour kGreen(46, 125, 50);
const wxColour kAmber(180, 120, 0);
const wxColour kCanvas(245, 246, 248);
const wxColour kInk(32, 33, 36);
const wxColour kMuted(107, 114, 128);
const wxColour kBorder(229, 231, 235);

wxString W(const std::string& text) { return wxString::FromUTF8(text.c_str()); }
std::string Utf8(const wxString& text) { const auto bytes = text.ToUTF8(); return bytes.data(); }

wxFileConfig* ReservationConfig() {
    const wxString directory = wxStandardPaths::Get().GetUserLocalDataDir();
    wxFileName::Mkdir(directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return new wxFileConfig("TaraShuttle", wxEmptyString,
                            directory + wxFILE_SEP_PATH + "reservations.ini",
                            wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
}

wxStaticText* AddText(wxWindow* parent, wxSizer* sizer, const wxString& text,
                      int flags = wxLEFT | wxRIGHT, int border = 10) {
    auto* label = new wxStaticText(parent, wxID_ANY, text);
    sizer->Add(label, 0, flags, border);
    return label;
}

wxPanel* MakeCard(wxWindow* parent, const wxString& title, wxBoxSizer*& inner) {
    auto* card = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    card->SetBackgroundColour(*wxWHITE);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* heading = new wxStaticText(card, wxID_ANY, title);
    heading->SetFont(wxFontInfo(11).Bold());
    heading->SetForegroundColour(kNavy);
    sizer->Add(heading, 0, wxALL, 12);
    inner = new wxBoxSizer(wxVERTICAL);
    sizer->Add(inner, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    card->SetSizer(sizer);
    return card;
}

wxPanel* MetricCard(wxWindow* parent, const wxString& title, const wxString& value,
                    const wxString& note, const wxColour& accent) {
    auto* card = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    card->SetBackgroundColour(*wxWHITE);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* name = new wxStaticText(card, wxID_ANY, title);
    name->SetForegroundColour(accent);
    name->SetFont(wxFontInfo(9).Bold());
    auto* number = new wxStaticText(card, wxID_ANY, value);
    number->SetForegroundColour(kNavy);
    number->SetFont(wxFontInfo(22).Bold());
    auto* caption = new wxStaticText(card, wxID_ANY, note);
    caption->SetForegroundColour(kMuted);
    sizer->Add(name, 0, wxALL, 12);
    sizer->Add(number, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    sizer->Add(caption, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    card->SetSizer(sizer);
    return card;
}

wxButton* MakeButton(wxWindow* parent, const wxString& label, bool primary = false) {
    auto* button = new wxButton(parent, wxID_ANY, label);
    button->SetMinSize(wxSize(-1, 36));
    if (primary) {
        button->SetBackgroundColour(kBlue);
        button->SetForegroundColour(*wxWHITE);
    }
    return button;
}

wxListCtrl* MakeTable(wxWindow* parent, const std::vector<wxString>& columns,
                      const std::vector<int>& widths = {}) {
    auto* table = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 260),
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    table->SetBackgroundColour(*wxWHITE);
    table->SetTextColour(kInk);
    for (size_t i = 0; i < columns.size(); ++i) {
        const int width = i < widths.size() ? widths[i] : 130;
        table->InsertColumn(static_cast<long>(i), columns[i], wxLIST_FORMAT_LEFT, width);
    }
    return table;
}

long AddRow(wxListCtrl* table, long data, const std::vector<wxString>& values) {
    // Keep both conditional branches as wxString for the InsertItem overload.
    const wxString firstColumn = values.empty() ? wxString{} : values[0];
    const long row = table->InsertItem(table->GetItemCount(), firstColumn);
    for (size_t i = 1; i < values.size(); ++i)
        table->SetItem(row, static_cast<int>(i), values[i]);
    table->SetItemData(row, static_cast<wxUIntPtr>(data));
    if (!values.empty()) {
        const wxString state = values.back().Lower();
        if (state.Contains("cancel") || state.Contains("full") || state.Contains("out of service"))
            table->SetItemTextColour(row, kRed);
        else if (state.Contains("pending") || state.Contains("boarding") || state.Contains("waiting"))
            table->SetItemTextColour(row, kAmber);
        else if (state.Contains("active") || state.Contains("available") || state.Contains("confirmed") || state.Contains("checked in") || state.Contains("completed"))
            table->SetItemTextColour(row, kGreen);
    }
    return row;
}

wxString JoinPipe(const std::vector<std::string>& values) {
    wxString line;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) line += "|";
        line += W(values[i]);
    }
    return line;
}

std::vector<wxString> SplitPipe(const wxString& input) {
    std::vector<wxString> parts;
    wxString token;
    for (wxUniChar c : input) {
        if (c == '|') { parts.push_back(token.Strip(wxString::both)); token.clear(); }
        else token += c;
    }
    parts.push_back(token.Strip(wxString::both));
    return parts;
}

bool Includes(const std::string& value, const wxString& query) {
    if (query.empty()) return true;
    wxString a = W(value).Lower();
    wxString b = query.Lower();
    return a.Contains(b);
}

bool IsActiveReservation(const ReservationRecord& reservation) {
    return reservation.status != "Cancelled";
}

bool HasAllocatedSeats(const ReservationRecord& reservation) {
    return IsActiveReservation(reservation) && reservation.status != "Waitlisted" &&
           reservation.seat != "--";
}

bool IsPastDepartureCutoff(const wxString& departure, int minutes) {
    const wxString dateText = departure.BeforeFirst(' ').Strip();
    const wxString timeText = departure.AfterFirst(' ').Strip();
    if (dateText.empty() || timeText.empty()) return false;
    wxDateTime date;
    wxDateTime time;
    if (!date.ParseISODate(dateText) || !time.ParseTime(timeText)) return false;
    date.SetHour(time.GetHour());
    date.SetMinute(time.GetMinute());
    date.SetSecond(0);
    return wxDateTime::Now() >= date.Subtract(wxTimeSpan(0, minutes, 0));
}

bool IsPaymentCleared(const ReservationRecord& reservation) {
    return reservation.paymentStatus == "Paid (demo)" ||
           reservation.paymentStatus == "Verified (demo)" ||
           reservation.paymentStatus == "Cash received (demo)";
}

bool IsBookableTrip(const ScheduleRecord& trip) {
    return trip.availableSeats > 0 && trip.status != "Full" &&
           trip.status != "On Route" && trip.status != "Completed";
}

bool IsAssignedToDriver(const ScheduleRecord& trip, const std::string& driverId, const std::string& driverName) {
    return !trip.driverId.empty() ? trip.driverId == driverId : trip.driver == driverName;
}

std::string AllocateSeatRange(const std::vector<ReservationRecord>& reservations,
                              int scheduleIndex, int seatCount, int capacity) {
    std::vector<bool> occupied(static_cast<size_t>(capacity + 1), false);
    for (const auto& reservation : reservations) {
        if (!HasAllocatedSeats(reservation) || reservation.scheduleIndex != scheduleIndex) continue;
        std::vector<int> numbers;
        for (size_t i = 0; i < reservation.seat.size();) {
            if (!std::isdigit(static_cast<unsigned char>(reservation.seat[i]))) { ++i; continue; }
            int value = 0;
            while (i < reservation.seat.size() && std::isdigit(static_cast<unsigned char>(reservation.seat[i])))
                value = value * 10 + (reservation.seat[i++] - '0');
            numbers.push_back(value);
        }
        if (numbers.size() > 1) {
            for (int seat = std::max(1, std::min(numbers.front(), numbers.back()));
                 seat <= std::min(capacity, std::max(numbers.front(), numbers.back())); ++seat)
                occupied[static_cast<size_t>(seat)] = true;
        } else if (!numbers.empty() && numbers.front() > 0 && numbers.front() <= capacity) {
            occupied[static_cast<size_t>(numbers.front())] = true;
        }
    }
    for (int first = 1; first + seatCount - 1 <= capacity; ++first) {
        bool free = true;
        for (int seat = first; seat < first + seatCount; ++seat)
            if (occupied[static_cast<size_t>(seat)]) { free = false; break; }
        if (free) return seatCount == 1 ? "A" + std::to_string(first)
                                      : "A" + std::to_string(first) + "-A" + std::to_string(first + seatCount - 1);
    }
    return {};
}

} // namespace
