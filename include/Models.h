#pragma once

#include <string>
#include <vector>

struct RouteRecord {
    std::string id, name, pickup, destination, shuttle, travelTime, status;
};

struct ScheduleRecord {
    // ScheduleRecord is the shared demo trip record; driver assignment is the Driver ID/name.
    std::string id, routeId, departure, arrival, driver, shuttle, status;
    int availableSeats{0};
    int capacity{0};
    std::string driverId;
};

// TODO [Kevin Miguel A. Antido]: Implement the reservation FIFO queue and hash tables for fast ID lookups;
// handle duplicate keys, missing keys, waitlisting, and seat reallocation.
// TODO [Alejandro Antonio T. Karunugan]: Define and document the system architecture and shared data-structure interfaces;
// coordinate queue/hash integration across reservations, schedules, payments, and boarding.
struct ReservationRecord {
    std::string id, passenger, studentId, route, departure, seat;
    std::string paymentMethod, paymentStatus, status;
    int seatCount{1};
    int scheduleIndex{-1};
    std::string paymentReference;
    std::string boardingStatus{"Waiting"};
};

struct PassengerRecord {
    std::string studentId, name, department, contact;
};

struct DriverRecord {
    std::string id, name, shuttle, route, contact, availability;
};

inline std::vector<RouteRecord> SampleRoutes() {
    // TODO [John Angelo V. Yboa]: Model connected campus pickup points as a graph and implement/test BFS route traversal.
    return {
        {"RT-01", "Library Loop", "Main Campus", "Library", "SH-001", "12 min", "Active"},
        {"RT-02", "Dormitory Express", "Main Campus", "Dormitory", "SH-002", "18 min", "Active"},
        {"RT-03", "Town Center Line", "Main Campus", "Town Center", "SH-003", "24 min", "Active"},
        {"RT-04", "Student Center Link", "Main Campus", "Student Center", "SH-004", "9 min", "Active"},
        {"RT-05", "Sports Complex Route", "Main Campus", "Sports Complex", "SH-005", "15 min", "Paused"}
    };
}

inline std::vector<ScheduleRecord> SampleSchedules() {
    return {
        {"SC-101", "RT-01", "8:30 AM", "8:42 AM", "Juan Dela Cruz", "SH-001", "On Route", 17, 18, "DR-001"},
        {"SC-102", "RT-02", "9:00 AM", "9:18 AM", "Maria Santos", "SH-002", "Boarding", 17, 18, "DR-002"},
        {"SC-103", "RT-03", "9:30 AM", "9:54 AM", "Carlo Mendoza", "SH-003", "Available", 17, 18, "DR-003"},
        {"SC-104", "RT-04", "10:00 AM", "10:09 AM", "Rina Flores", "SH-004", "Available", 18, 18, "DR-004"},
        {"SC-105", "RT-01", "10:30 AM", "10:42 AM", "Juan Dela Cruz", "SH-001", "Available", 18, 18, "DR-001"}
    };
}

inline std::vector<ReservationRecord> SampleReservations() {
    return {
        {"RS-24018", "Anton Karunungan", "2025164253", "Library Loop", "8:30 AM", "A4", "Cash", "Pending", "Confirmed", 1, 0, "", "Waiting"},
        {"RS-24021", "Jamie Cruz", "S-2021-0820", "Town Center Line", "9:30 AM", "B2", "GCash", "Awaiting review (demo)", "Pending", 1, 2, "DEMO-GC-24021", "Waiting"},
        {"RS-24025", "Taylor Lim", "S-2023-0221", "Dormitory Express", "9:00 AM", "C1", "Cash", "Paid (demo)", "Confirmed", 1, 1, "", "Waiting"},
        {"RS-24028", "Morgan Santos", "S-2022-0437", "Library Loop", "10:30 AM", "--", "GCash", "Not paid", "Cancelled", 1, 4, "", "Waiting"}
    };
}

inline std::vector<PassengerRecord> SamplePassengers() {
    return {
        {"2025164253", "Anton Karunungan", "Information Technology", "555-0101"},
        {"S-2021-0820", "Jamie Cruz", "Information Technology", "555-0102"},
        {"S-2023-0221", "Taylor Lim", "Engineering", "555-0103"},
        {"S-2022-0437", "Morgan Santos", "Business", "555-0104"},
        {"S-2024-0065", "Riley Garcia", "Design", "555-0105"}
    };
}

inline std::vector<DriverRecord> SampleDrivers() {
    return {
        {"DR-001", "Juan Dela Cruz", "SH-001", "Library Loop", "555-0201", "Available"},
        {"DR-002", "Maria Santos", "SH-002", "Dormitory Express", "555-0202", "On Route"},
        {"DR-003", "Carlo Mendoza", "SH-003", "Town Center Line", "555-0203", "Available"},
        {"DR-004", "Rina Flores", "SH-004", "Student Center Link", "555-0204", "Off Duty"}
    };
}

