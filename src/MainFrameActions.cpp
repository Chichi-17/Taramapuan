#include "MainFrame.h"
#include "ReservationDialog.h"
#include "UiHelpers.h"

#include <wx/dateevt.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <cstddef>
#include <utility>

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
       draft.scheduleIndex<0||draft.scheduleIndex>=static_cast<int>(m_schedules.size())||
       draft.seatCount<1){
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
        return reservation.studentId == studentId && reservation.scheduleIndex == draft.scheduleIndex &&
               IsActiveReservation(reservation);
    });
    if (duplicate != m_reservations.end()) {
        wxMessageBox("This student already has an active reservation for the selected departure.", "Duplicate reservation", wxOK | wxICON_WARNING, this);
        return;
    }
    const bool waitlistRequested=draft.joinWaitlist&&draft.seatCount>schedule.availableSeats;
    if(draft.seatCount>schedule.availableSeats&&!waitlistRequested){
        wxMessageBox(wxString::Format("Only %d seat(s) remain on this departure.",schedule.availableSeats),"Not enough seats",wxOK|wxICON_WARNING,this);return;
    }
    const std::string id="TS-"+std::to_string(m_reservationSequence++);
    const std::string payment=Utf8(draft.paymentMethod);
    const std::string status="Pending driver approval";
    const std::string paymentStatus="Not due until approved";
    ReservationRecord reservation{id,Utf8(draft.passenger),studentId,
        m_routes[static_cast<size_t>(draft.routeIndex)].name,
        Utf8(draft.travelDate)+"  "+schedule.departure,"--",payment,paymentStatus,status,
        draft.seatCount,draft.scheduleIndex,Utf8(draft.paymentReference),"Waiting",draft.joinWaitlist};
    m_reservations.push_back(reservation);
    SaveReservations();
    m_selectedReservationId = id;
    wxMessageBox(wxString::Format("Request %s was sent to the driver assigned to %s. No seat or payment is held until approval.",
        W(id), W(schedule.id)),
                 "Reservation created",wxOK|wxICON_INFORMATION,this);
    SetStatusText("Local preview updated  |  Sample reservation "+W(id));
    RefreshCurrentPage();
    // TODO: Add reservation cutoff/no-show handling, notification delivery, and an auditable reservation event log.
}


void MainFrame::ReviewReservationRequest(long rowIndex, bool approved) {
    if (rowIndex < 0 || rowIndex >= static_cast<long>(m_reservations.size())) return;
    auto& reservation = m_reservations[static_cast<size_t>(rowIndex)];
    if (m_userRole != UserRole::Driver || m_selectedDriver < 0 ||
        m_selectedDriver >= static_cast<int>(m_drivers.size())) {
        wxMessageBox("Only a signed-in driver can review reservation requests.",
                     "Access unavailable", wxOK | wxICON_WARNING, this);
        return;
    }
    if (reservation.status != "Pending driver approval" ||
        reservation.scheduleIndex < 0 ||
        reservation.scheduleIndex >= static_cast<int>(m_schedules.size())) {
        wxMessageBox("This reservation is not awaiting driver approval.",
                     "Request unavailable", wxOK | wxICON_INFORMATION, this);
        return;
    }
    auto& schedule = m_schedules[static_cast<size_t>(reservation.scheduleIndex)];
    const auto& driver = m_drivers[static_cast<size_t>(m_selectedDriver)];
    if (!IsAssignedToDriver(schedule, driver.id, driver.name)) {
        wxMessageBox("Only the driver assigned to this departure can review the request.",
                     "Request assigned to another driver", wxOK | wxICON_WARNING, this);
        return;
    }
    if (IsPastDepartureCutoff(W(reservation.departure), 10)) {
        ProcessWaitlistCutoffs();
        wxMessageBox("Driver approval closes 10 minutes before departure.",
                     "Approval cutoff passed", wxOK | wxICON_WARNING, this);
        return;
    }

    if (!approved) {
        reservation.status = "Rejected by driver";
        reservation.paymentStatus = "Not due - request rejected";
        reservation.boardingStatus = "Rejected";
    } else if (reservation.seatCount <= schedule.availableSeats) {
        const std::string seat = AllocateSeatRange(m_reservations, reservation.scheduleIndex,
                                                    reservation.seatCount, schedule.capacity);
        if (seat.empty()) {
            wxMessageBox("No valid seat assignment can be made for this request.",
                         "Approval unavailable", wxOK | wxICON_WARNING, this);
            return;
        }
        reservation.seat = seat;
        reservation.status = "Confirmed (demo)";
        reservation.paymentStatus = reservation.paymentMethod == "Cash"
            ? "Due on board" : "Awaiting review (demo)";
        schedule.availableSeats -= reservation.seatCount;
        if (schedule.availableSeats == 0) schedule.status = "Full";
        reservation.boardingStatus = "Waiting";
    } else if (reservation.waitlistRequested) {
        reservation.status = "Waitlisted";
        reservation.paymentStatus = "Not due until a seat is offered";
        reservation.seat = "--";
        reservation.boardingStatus = "Waiting";
        m_reservationQueue.enqueue(reservation);
    } else {
        wxMessageBox("There are no longer enough seats. The student did not request a waitlist.",
                     "Approval unavailable", wxOK | wxICON_INFORMATION, this);
        return;
    }
    SaveReservations();
    SetStatusText(approved ? "Reservation request approved by its assigned driver."
                           : "Reservation request rejected by its assigned driver.");
    RefreshCurrentPage();
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
    const std::string departure=reservation.departure;
    const int releasedSeats=HasAllocatedSeats(reservation)?reservation.seatCount:0;
    reservation.status="Cancelled";
    reservation.paymentStatus="Not collected";
    reservation.boardingStatus="Cancelled";
    if(releasedSeats>0){
        schedule.availableSeats=std::min(schedule.capacity,schedule.availableSeats+releasedSeats);
        if(schedule.availableSeats>0&&schedule.status=="Full")schedule.status="Available";
    }

    int promoted=0;
    while(!m_reservationQueue.isEmpty(scheduleIndex,departure)){
        const ReservationRecord next=m_reservationQueue.front(scheduleIndex,departure);
        auto waiting=std::find_if(m_reservations.begin(),m_reservations.end(),[&](const ReservationRecord& item){
            return item.id==next.id;
        });
        if(waiting==m_reservations.end()||waiting->status!="Waitlisted"){
            m_reservationQueue.dequeue(scheduleIndex,departure);
            continue;
        }
        if(next.seatCount>schedule.availableSeats)break;
        const std::string seat=AllocateSeatRange(m_reservations,scheduleIndex,next.seatCount,schedule.capacity);
        if(seat.empty())break;
        m_reservationQueue.dequeue(scheduleIndex,departure);
        waiting->seat=seat;
        waiting->status="Confirmed (demo)";
        waiting->paymentStatus=waiting->paymentMethod=="Cash"?"Due on board":"Payment due after seat offer";
        waiting->boardingStatus="Waiting";
        schedule.availableSeats-=waiting->seatCount;
        ++promoted;
    }
    if(schedule.availableSeats==0)schedule.status="Full";
    SaveReservations();
    SetStatusText(promoted>0?wxString::Format("Reservation cancelled; %d waitlisted passenger(s) received a seat offer.",promoted):wxString("Sample reservation cancelled; its seats were released."));
    wxMessageBox(promoted>0?wxString::Format("Reservation cancelled. %d waitlisted passenger(s) were offered a seat.",promoted):wxString("Reservation cancelled. Its seat(s) were returned to the schedule."),
                 "Cancellation complete",wxOK|wxICON_INFORMATION,this);
    RefreshCurrentPage();
}


void MainFrame::RegisterWalkIn(int scheduleIndex) {
    if (m_userRole != UserRole::Driver || m_selectedDriver < 0 ||
        m_selectedDriver >= static_cast<int>(m_drivers.size()) ||
        scheduleIndex < 0 || scheduleIndex >= static_cast<int>(m_schedules.size())) {
        wxMessageBox("Select an assigned departure while signed in as a driver.",
                     "Walk-in unavailable", wxOK | wxICON_WARNING, this);
        return;
    }

    auto& schedule = m_schedules[static_cast<size_t>(scheduleIndex)];
    const auto& driver = m_drivers[static_cast<size_t>(m_selectedDriver)];
    if (!IsAssignedToDriver(schedule, driver.id, driver.name) ||
        schedule.status == "On Route" || schedule.status == "Completed") {
        wxMessageBox("This departure is not available for walk-in registration.",
                     "Walk-in unavailable", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxTextEntryDialog dateDialog(this, "Enter the travel date (YYYY-MM-DD).",
                                 "Walk-in travel date", wxDateTime::Today().FormatISODate());
    if (dateDialog.ShowModal() != wxID_OK) return;
    const wxString departure = dateDialog.GetValue().Strip(wxString::both) + " " + W(schedule.departure);
    wxDateTime departureTime;
    if (!ParseDeparture(departure, departureTime) ||
        wxDateTime::Now() < departureTime.Subtract(wxTimeSpan(0, 10, 0)) ||
        wxDateTime::Now() >= departureTime) {
        wxMessageBox("Walk-in registration opens 10 minutes before departure and closes at departure.",
                     "Walk-in time window", wxOK | wxICON_WARNING, this);
        return;
    }
    ProcessWaitlistCutoffs();
    if (schedule.availableSeats <= 0) {
        wxMessageBox("No seat is available for this walk-in.", "Walk-in unavailable",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxTextEntryDialog nameDialog(this, "Enter the walk-in student's name.", "Walk-in student");
    if (nameDialog.ShowModal() != wxID_OK) return;
    const std::string passenger = Utf8(nameDialog.GetValue().Strip(wxString::both));
    wxTextEntryDialog idDialog(this, "Enter the walk-in student's ID.", "Walk-in student ID");
    if (idDialog.ShowModal() != wxID_OK) return;
    const std::string studentId = Utf8(idDialog.GetValue().Strip(wxString::both));
    if (passenger.empty() || studentId.empty()) {
        wxMessageBox("A student name and ID are required.", "Walk-in details required",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    const auto duplicate = std::find_if(m_reservations.begin(), m_reservations.end(),
        [&](const ReservationRecord& reservation) {
            return reservation.studentId == studentId &&
                   reservation.scheduleIndex == scheduleIndex &&
                   reservation.departure == Utf8(departure) &&
                   IsActiveReservation(reservation);
        });
    if (duplicate != m_reservations.end()) {
        wxMessageBox("This student already has an active record for this departure.",
                     "Duplicate walk-in", wxOK | wxICON_WARNING, this);
        return;
    }

    const auto route = std::find_if(m_routes.begin(), m_routes.end(),
        [&](const RouteRecord& item) { return item.id == schedule.routeId; });
    if (route == m_routes.end()) {
        wxMessageBox("The route for this departure could not be found.",
                     "Walk-in unavailable", wxOK | wxICON_ERROR, this);
        return;
    }
    const std::string seat = AllocateSeatRange(m_reservations, scheduleIndex, 1, schedule.capacity);
    if (seat.empty()) {
        wxMessageBox("No seat is available for this walk-in.", "Walk-in unavailable",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    const std::string id = "TS-" + std::to_string(m_reservationSequence++);
    m_reservations.push_back({id, passenger, studentId, route->name,
        Utf8(departure), seat, "Cash", "Cash received (demo)", "Confirmed (demo)",
        1, scheduleIndex, "", "Boarded"});
    --schedule.availableSeats;
    if (schedule.availableSeats == 0) schedule.status = "Full";
    SaveReservations();
    SetStatusText("On-site walk-in registered and checked in for " + W(schedule.id) + ".");
    RefreshCurrentPage();
}


void MainFrame::AddOrEditRoute(long routeIndex){
    if(m_userRole!=UserRole::Driver){wxMessageBox("Only a signed-in driver can register or edit routes.","Access unavailable",wxOK|wxICON_WARNING,this);return;}
    const bool editing=routeIndex>=0&&routeIndex<static_cast<long>(m_routes.size());std::vector<std::string> values;
    if(editing){const auto&r=m_routes[static_cast<size_t>(routeIndex)];values={r.id,r.name,r.pickup,r.destination,r.shuttle,r.travelTime,r.status};}
    else{
        std::string nextId;
        for(int number=1;nextId.empty();++number){
            const std::string candidate=std::string("RT-")+(number<10?"0":"")+std::to_string(number);
            if(std::none_of(m_routes.begin(),m_routes.end(),[&](const RouteRecord& route){return route.id==candidate;}))nextId=candidate;
        }
        values={nextId,"New Campus Loop","Main Campus","New Stop","SH-006","15 min","Active"};
    }
    const wxString prompt="Enter 7 values separated by |:\nID | Name | Pickup | Destination | Shuttle | Travel time | Status\nUse Active to publish this route to students, or Paused to hide it.";
    wxTextEntryDialog dialog(this,prompt,editing?"Edit Route":"Add Route",JoinPipe(values));if(dialog.ShowModal()!=wxID_OK)return;auto fields=SplitPipe(dialog.GetValue());if(fields.size()!=7){wxMessageBox("Enter exactly 7 pipe-separated values.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    for(const auto&f:fields)if(f.empty()){wxMessageBox("Route fields cannot be blank.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    RouteRecord record{Utf8(fields[0]),Utf8(fields[1]),Utf8(fields[2]),Utf8(fields[3]),Utf8(fields[4]),Utf8(fields[5]),Utf8(fields[6])};
    if(record.status!="Active"&&record.status!="Paused"){wxMessageBox("Route status must be Active or Paused.","Invalid route status",wxOK|wxICON_WARNING,this);return;}
    for(size_t i=0;i<m_routes.size();++i)if((!editing||i!=static_cast<size_t>(routeIndex))&&m_routes[i].id==record.id){wxMessageBox("Route IDs must be unique.","Duplicate route ID",wxOK|wxICON_WARNING,this);return;}
    if(editing){
        const std::string previousId=m_routes[static_cast<size_t>(routeIndex)].id;
        if(previousId!=record.id&&std::any_of(m_schedules.begin(),m_schedules.end(),[&](const ScheduleRecord& schedule){return schedule.routeId==previousId;})){
            wxMessageBox("A route with published departures cannot change its ID.","Route ID is in use",wxOK|wxICON_WARNING,this);return;
        }
        m_routes[static_cast<size_t>(routeIndex)]=std::move(record);
    }else m_routes.push_back(std::move(record));
    SaveRoutesAndSchedules();
    RefreshCurrentPage();
}


void MainFrame::AddSchedule(){
    if(m_userRole!=UserRole::Driver||m_selectedDriver<0||m_selectedDriver>=static_cast<int>(m_drivers.size())){wxMessageBox("Only a signed-in driver can publish a departure.","Access unavailable",wxOK|wxICON_WARNING,this);return;}
    const auto& currentDriver=m_drivers[static_cast<size_t>(m_selectedDriver)];
    std::string nextId;
    for(int number=101;nextId.empty();++number){
        const std::string candidate="SC-"+std::to_string(number);
        if(std::none_of(m_schedules.begin(),m_schedules.end(),[&](const ScheduleRecord& schedule){return schedule.id==candidate;}))nextId=candidate;
    }
    const auto firstActiveRoute=std::find_if(m_routes.begin(),m_routes.end(),[](const RouteRecord& route){return route.status=="Active";});
    const std::string defaultRoute=firstActiveRoute==m_routes.end()?std::string{}:firstActiveRoute->id;
    wxString prompt=wxString::Format("Enter 8 values separated by |:\nID | Route ID | Departure | Arrival | Shuttle | Status | Available seats | Capacity\nThis departure will be assigned to %s.\nActive routes:",W(currentDriver.name));
    for(const auto& route:m_routes)if(route.status=="Active"){prompt+="\n";prompt+=W(route.id);prompt+=" - ";prompt+=W(route.name);}
    const wxString defaults=W(nextId+"|"+defaultRoute+"|11:00 AM|11:12 AM|"+currentDriver.shuttle+"|Available|18|18");
    wxTextEntryDialog dialog(this,prompt,"Publish Departure",defaults);
    if(dialog.ShowModal()!=wxID_OK)return;
    auto f=SplitPipe(dialog.GetValue());
    if(f.size()!=8){wxMessageBox("Enter exactly 8 pipe-separated values.","Invalid schedule",wxOK|wxICON_WARNING,this);return;}
    for(const auto&field:f)if(field.empty()){wxMessageBox("Schedule fields cannot be blank.","Invalid schedule",wxOK|wxICON_WARNING,this);return;}
    const std::string id=Utf8(f[0]),routeId=Utf8(f[1]),departure=Utf8(f[2]),arrival=Utf8(f[3]),shuttle=Utf8(f[4]);
    auto route=std::find_if(m_routes.begin(),m_routes.end(),[&](const RouteRecord&r){return r.id==routeId;});
    if(route==m_routes.end()||route->status!="Active"){wxMessageBox("Choose an existing active route ID.","Invalid route",wxOK|wxICON_WARNING,this);return;}
    long seats=0,capacity=0;
    if(!f[6].ToLong(&seats)||!f[7].ToLong(&capacity)||seats<0||capacity<1||seats>capacity){wxMessageBox("Seat counts must be valid and available seats cannot exceed capacity.","Invalid capacity",wxOK|wxICON_WARNING,this);return;}
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
    m_schedules.push_back({id,routeId,departure,arrival,currentDriver.name,shuttle,Utf8(f[5]),static_cast<int>(seats),static_cast<int>(capacity),currentDriver.id});
    SaveRoutesAndSchedules();
    RefreshCurrentPage();
}


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

void MainFrame::ReviewPayment(long rowIndex) { SetPaymentStatus(rowIndex,true); }


void MainFrame::SetPaymentStatus(long rowIndex, bool accepted) {
    if(rowIndex<0||rowIndex>=static_cast<long>(m_reservations.size()))return;
    auto& reservation=m_reservations[static_cast<size_t>(rowIndex)];
    if(m_userRole!=UserRole::Driver){wxMessageBox("Only a signed-in driver can review payments.","Access unavailable",wxOK|wxICON_WARNING,this);return;}
    if(!IsActiveReservation(reservation)){wxMessageBox("Cancelled reservations cannot be verified.","Payment unavailable",wxOK|wxICON_WARNING,this);return;}
    if(reservation.status=="Pending driver approval"){wxMessageBox("Payment review is available only after the assigned driver approves the request.","Payment unavailable",wxOK|wxICON_INFORMATION,this);return;}
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
    if(reservation.status=="Pending driver approval"){wxMessageBox("The assigned driver must approve this request before check-in.","Boarding unavailable",wxOK|wxICON_INFORMATION,this);return;}
    if(reservation.status=="Waitlisted"){wxMessageBox("A boarding pass is unavailable until a seat is offered.","Boarding unavailable",wxOK|wxICON_INFORMATION,this);return;}
    if(reservation.scheduleIndex<0||reservation.scheduleIndex>=static_cast<int>(m_schedules.size())||!IsAssignedToDriver(m_schedules[static_cast<size_t>(reservation.scheduleIndex)],m_drivers[static_cast<size_t>(m_selectedDriver)].id,m_drivers[static_cast<size_t>(m_selectedDriver)].name)){wxMessageBox("This passenger is not on your assigned trips.","Boarding unavailable",wxOK|wxICON_WARNING,this);return;}
    if(boarded&&reservation.boardingStatus=="Boarded"){wxMessageBox("This passenger has already been checked in.","Already boarded",wxOK|wxICON_INFORMATION,this);return;}
    if(boarded&&IsPastDepartureCutoff(W(reservation.departure),10)){ProcessWaitlistCutoffs();wxMessageBox("Check-in closes 10 minutes before departure. This seat is now open to on-site walk-ins.","Check-in cutoff passed",wxOK|wxICON_WARNING,this);return;}
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
    trip.status=completed?"Completed":"On Route";SaveRoutesAndSchedules();SetStatusText("Trip status updated for the assigned demo trip.");RefreshCurrentPage();
}
