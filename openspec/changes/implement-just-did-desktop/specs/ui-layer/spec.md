## ADDED Requirements

### Requirement: Floating input window

The system SHALL display a frameless, always-on-top floating window at the right side of the screen with collapsed (title only) and expanded (input + submit) states.

#### Scenario: Click title toggles expand

- **WHEN** user clicks on the floating window in collapsed state
- **THEN** the window expands to show input area and submit button

#### Scenario: Click blank area collapses

- **WHEN** user clicks on the blank area of the expanded floating window
- **THEN** the window collapses back to title-only state

#### Scenario: Submit record from floating window

- **WHEN** user enters text in the input area and clicks submit
- **THEN** the record is saved via ReportService and input is cleared

#### Scenario: Double click shows main window

- **WHEN** user double-clicks the floating window
- **THEN** the main management window opens and the floating window hides

### Requirement: Main management window

The system SHALL provide a main window with a calendar view for date navigation, a timeline view for daily records, and a bottom status bar for connection state.

#### Scenario: Calendar shows days with reports

- **WHEN** the main window loads the current month
- **THEN** days with report files are visually marked based on `pc_daliy_report_index` data

#### Scenario: Click a date shows records

- **WHEN** user clicks a date in the calendar that has a report
- **THEN** the timeline view below displays all records for that date in time order

#### Scenario: Month navigation updates calendar

- **WHEN** user clicks previous month or next month button
- **THEN** the calendar refreshes to show marked days for the new month

### Requirement: Storage management

The system SHALL provide storage management with overview statistics, date range clearing, ZIP backup export, and ZIP restore.

#### Scenario: Storage stats display

- **WHEN** user opens the storage management page
- **THEN** total disk usage and per-year breakdown are displayed

#### Scenario: Backup export creates ZIP

- **WHEN** user selects a date range and clicks backup
- **THEN** a ZIP file is created with `backup_{YYYYMMDD}_{HHmmss}.zip` naming and internal `data/YYYY/MM/DD.txt` structure

#### Scenario: Clear date range removes data

- **WHEN** user selects dates and confirms clear operation
- **THEN** selected daily files and index entries are permanently removed

### Requirement: System tray integration

The system SHALL minimize to system tray with a context menu for opening the main window and quitting the application.

#### Scenario: Minimize to tray

- **WHEN** user closes the main window
- **THEN** the application minimizes to system tray and the floating window becomes visible

#### Scenario: Restore from tray

- **WHEN** user clicks "打开主窗口" in the tray context menu
- **THEN** the main window is restored and the floating window hides

### Requirement: Connection status display

The system SHALL display the current connection state (unstarted, disconnected, connected, syncing) in the status bar with visual indicator, device name, QR code, and start/stop button.

#### Scenario: QR code displayed when server is running

- **WHEN** the HTTP server is started
- **THEN** the status bar shows a QR code image for `http://{LAN IP}:{port}`

#### Scenario: Status updates on state change

- **WHEN** ConnectionStateMachine emits `stateChanged`
- **THEN** the status bar indicator color and text update immediately

### Requirement: ViewModel isolation from UI

All ViewModels SHALL communicate with the service layer only, never directly accessing core layer classes (DataManager, etc.) or HTTP objects.

#### Scenario: CalendarViewModel uses ReportService only

- **WHEN** reviewing `CalendarViewModel::loadMonth()` implementation
- **THEN** it calls `ReportService::getMonthIndex()` and does not include any core layer headers
