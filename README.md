# TaraMapuan

TaraMapuan is a C++17 desktop application built with wxWidgets and CMake. It keeps the existing dashboard shell, dark-blue sidebar, tables, and sample management pages while adding a landing page, role selection, demo login, and connected student/driver workflows.

## Demo access

- Student: `S-2022-0148` / `student123`
- Driver: `DR-001` / `driver123` (the other sample driver IDs use the same demo password)

The student and driver portals use the same in-memory reservation records. Reservation records are also saved to a local wxWidgets INI file by the existing demo persistence path. Routes, trips, drivers, and trip status changes remain sample data held in memory.

## Implemented demo workflows

- Students can browse bookable trips, create reservations, see their reservations and payment state, cancel eligible reservations, and view a digital-pass layout with an explicitly nonfunctional QR placeholder.
- Drivers see trips assigned to their demo driver ID, open the trip manifest, inspect payment references, mark cash as received, manually verify or reject an online reference, check in eligible passengers, and depart or complete trips under demo rules.
- Payment actions are manual labels only. No payment provider, campus API, production authentication, database, QR verification, or live GPS service is connected.

## Build (Windows / vcpkg)

Configure and build using the existing Visual Studio x64 toolchain:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/Users/acer/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DwxWidgets_DIR="C:/Users/acer/Downloads/vcpkg/installed/x64-windows/share/wxwidgets"
cmake --build build --config Release
```

Run the generated application with:

```powershell
.\build\Release\TaraShuttle.exe
```

## Source layout

- `src/main.cpp` — wxWidgets application entry point
- `src/MainFrame.cpp` — landing, login, navigation, student and driver pages, and demo workflows
- `src/ReservationDialog.cpp` — validated trip and payment-reference form
- `include/MainFrame.h` — frame, page, role, and event declarations
- `include/ReservationDialog.h` — reservation draft and dialog declarations
- `include/Models.h` — shared route, schedule, driver, manifest, and reservation records
- `CMakeLists.txt` — C++17 wxWidgets target configuration

## Team integration TODOs

- **Kevin Miguel A. Antido** — `include/Models.h`: FIFO queue, waitlist, seat reallocation, hash lookups, and duplicate/missing-key handling.
- **Alejandro Antonio T. Karunugan** — `include/Models.h`: shared architecture and interfaces; integrate queue/hash work and cross-workflow consistency tests.
- **Justin Axcel C. Magpily** — `src/MainFrame.cpp`, `src/ReservationDialog.cpp`: student portal UX, trip selection, reservation/cancellation refinements, capacity and duplicate rules, and digital-pass presentation.
- **Robby Ruth Abaquin** — `src/MainFrame.cpp`: driver dashboard/manifest refinements and dispatch rules; current demo manifest, payment actions, and boarding workflows are implemented.
- **Cleoh Amor B. Sarmiento** — `src/MainFrame.cpp`: Quick Sort for manifests and Binary Search only over correctly sorted lookup data; document algorithm behavior and test edge cases.
- **John Angelo V. Yboa** — `include/Models.h`, `README.md`: pickup-point graph and BFS tests; document data flow, algorithms, and backend limitations.

Additional TODO markers in the sources cover database persistence, real reservation processing, payment gateway verification, secure authentication, QR generation/verification, live GPS, notifications, audit logs, reservation cutoffs/no-shows, and reporting exports.
