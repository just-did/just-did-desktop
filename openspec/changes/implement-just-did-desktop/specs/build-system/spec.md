## ADDED Requirements

### Requirement: CMake project structure

The project SHALL use CMake 3.20+ as its build system with four static libraries (common, core, service, ui) and one executable (JustDid).

#### Scenario: Project builds from clean state

- **WHEN** developer runs `cmake -B build -S . -DCMAKE_PREFIX_PATH=<Qt path>` followed by `cmake --build build`
- **THEN** all four static libraries compile successfully and link into the JustDid executable

#### Scenario: yaml-cpp fetched automatically

- **WHEN** CMake configure runs for the first time
- **THEN** yaml-cpp 0.8.0 is fetched from GitHub via FetchContent and compiled as part of the build

### Requirement: Compile-time boundary enforcement

Each layer's CMakeLists.txt SHALL only link against its allowed dependencies, preventing accidental cross-layer imports.

#### Scenario: core layer cannot include Qt Quick

- **WHEN** core layer source file attempts `#include <QQuickItem>`
- **THEN** compilation fails because `justdid_core` does not link `Qt6::Quick`

#### Scenario: service layer cannot include ui headers

- **WHEN** service layer source file attempts `#include "ui/viewmodels/CalendarViewModel.h"`
- **THEN** compilation fails because `justdid_service` does not link `justdid_ui`

### Requirement: C++17 standard

The project SHALL compile with C++17 standard, as required by Qt 6.x minimum.

#### Scenario: C++17 features available

- **WHEN** source code uses `std::optional`, `std::string_view`, or structured bindings
- **THEN** compilation succeeds

### Requirement: Qt 6.8 module dependencies

The project SHALL depend on Qt6::Core, Qt6::Quick, Qt6::Network, Qt6::Sql, and Qt6::HttpServer.

#### Scenario: Missing Qt module detected

- **WHEN** a required Qt 6.8 module is not installed
- **THEN** CMake configure fails with a clear error message from `find_package(Qt6)`
