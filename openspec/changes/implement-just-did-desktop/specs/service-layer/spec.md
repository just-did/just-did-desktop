## ADDED Requirements

### Requirement: ReportService local daily report operations

The system SHALL provide a ReportService that handles all local daily report operations (add record, view, clear, stats, backup, restore) without touching HTTP or network.

#### Scenario: Add record generates timestamp

- **WHEN** `ReportService::addRecord(2026, 7, 23, "完成需求文档")` is called
- **THEN** the current time is added as HH:MM format and the record is persisted via DataManager

#### Scenario: Get month index returns marked days

- **WHEN** `ReportService::getMonthIndex(2026, 7)` is called
- **THEN** a list of IndexEntry is returned for all days in July 2026 that have report files

#### Scenario: Clear date removes file and index

- **WHEN** `ReportService::clearDate(2026, 7, 23)` is called
- **THEN** the daily file and its index entry are both removed

### Requirement: SyncService submit body parsing

The system SHALL parse the `/sync/submit` request body format (date blocks separated by triple newlines, records by double newlines, time and content by first newline) into structured data.

#### Scenario: Parse multi-date submit body

- **WHEN** `SyncService::submit()` receives a body with two date blocks (20260723 with 2 records, 20260724 with 1 record)
- **THEN** parseSubmitBody returns a QMap with two date keys and correct record counts

#### Scenario: Idempotent batch returns success

- **WHEN** `SyncService::submit()` is called with an already-processed batchId
- **THEN** it returns `code=0` with `updated_index` without re-merging data

#### Scenario: Version conflict returns error code -4

- **WHEN** DataManager::mergeRecords returns VersionConflict
- **THEN** SyncService returns `{"code": -4, "message": "数据版本冲突，请重试"}`

### Requirement: SyncService fetch response assembly

The system SHALL process `/sync/fetch` requests, validate date ranges (max 32 files, max 31 day span), and return content in the correct format (text/plain for single file, application/zip for multiple files).

#### Scenario: Single date fetch returns text

- **WHEN** `SyncService::fetch()` is called with a single date that has a report
- **THEN** the response is marked as text/plain with the file content as body

#### Scenario: Multi-date fetch returns zip

- **WHEN** `SyncService::fetch()` is called with 3 dates
- **THEN** the response is marked as application/zip containing all 3 files in data/YYYY/MM/DD.txt structure

#### Scenario: Too many files returns error

- **WHEN** `SyncService::fetch()` is called with 33 dates
- **THEN** it returns `code=-2` (TooManyFiles)

#### Scenario: All files missing returns 404

- **WHEN** `SyncService::fetch()` is called with dates that have no report files
- **THEN** it returns `code=-1` with HTTP 404 status

### Requirement: HttpServer route registration

The system SHALL register four HTTP routes: POST `/sync/connect`, GET `/health`, POST `/sync/submit`, POST `/sync/fetch`.

#### Scenario: Health check returns ok

- **WHEN** `GET /health` is received
- **THEN** response is `{"status": "ok", "timestamp": "<ISO 8601>"}` with HTTP 200

#### Scenario: Connect establishes connection

- **WHEN** `POST /sync/connect` is received with valid JSON body `{"device_name": "iPhone"}`
- **THEN** ConnectionStateMachine transitions to Connected and response includes `server_info`

#### Scenario: Submit extracts X-Batch-ID header

- **WHEN** `POST /sync/submit` is received with `X-Batch-ID: uuid-123` header
- **THEN** the batch ID is extracted and passed to SyncService without SyncService touching the HTTP layer

### Requirement: HttpServer QR code URL generation

The system SHALL generate a QR code URL string in the format `http://{local IP}:{port}` using the active LAN IP address.

#### Scenario: QR code URL contains LAN IP

- **WHEN** `HttpServer::qrCodeUrl()` is called while the server is listening on port 18080
- **THEN** the returned URL matches pattern `http://<valid LAN IP>:18080`
