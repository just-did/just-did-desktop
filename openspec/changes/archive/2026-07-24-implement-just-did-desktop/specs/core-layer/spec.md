## ADDED Requirements

### Requirement: LogManager thread-safe logging with rotation

The system SHALL provide a singleton LogManager that writes formatted log messages to `logs/just-did.log`, with automatic rotation at 4MB per file and 12MB total capacity limit.

#### Scenario: Log file rotates at 4MB

- **WHEN** the current log file reaches 4MB in size
- **THEN** the file is renamed to `just-did.1.log`, existing `just-did.1.log` becomes `just-did.2.log`, and a new `just-did.log` is created

#### Scenario: Old log deleted at 12MB total

- **WHEN** total size of `just-did.log` + `just-did.1.log` + `just-did.2.log` exceeds 12MB after rotation
- **THEN** the oldest file (`just-did.2.log`) is deleted

#### Scenario: Thread-safe concurrent writes

- **WHEN** multiple threads call `LogManager::info()` simultaneously
- **THEN** no data corruption or interleaved messages occur

### Requirement: ConfigManager YAML configuration

The system SHALL read and write `config.yml` using yaml-cpp, with defaults for all values when the file is missing.

#### Scenario: First launch creates default config

- **WHEN** the application starts and `config.yml` does not exist
- **THEN** `ConfigManager::load()` returns true with default values (port=18080, heartbeat_timeout=45)

#### Scenario: Config values are read correctly

- **WHEN** `config.yml` contains `server.port: 9090`
- **THEN** `ConfigManager::port()` returns 9090

#### Scenario: Config writes persist to file

- **WHEN** `ConfigManager::setPort(9090)` followed by `ConfigManager::save()`
- **THEN** the `config.yml` file on disk contains `port: 9090`

### Requirement: DatabaseManager SQLite operations

The system SHALL manage `pc_daliy_report_index` and `pc_processed_batches` tables through a dedicated DatabaseManager class.

#### Scenario: Tables created on first open

- **WHEN** `DatabaseManager::open("just_do.db")` is called and the database file does not exist
- **THEN** both tables are created with correct schema and WAL journal mode is enabled

#### Scenario: Upsert index entry

- **WHEN** `upsertIndexEntry(2026, 7, 23, "2026/07/23.txt", 256)` is called for a new date
- **THEN** a new row is inserted; when called again for the same date, the existing row is updated

#### Scenario: Optimistic lock update succeeds

- **WHEN** `updateWithVersion(2026, 7, 23, "2026/07/23.txt", 512, 1)` is called and the current version in DB is 1
- **THEN** the update succeeds (returns true), version becomes 2

#### Scenario: Optimistic lock update fails on version mismatch

- **WHEN** `updateWithVersion(2026, 7, 23, "2026/07/23.txt", 512, 1)` is called but the current version in DB is 2
- **THEN** the update fails (returns false), data is unchanged

#### Scenario: Batch idempotency check

- **WHEN** `isBatchProcessed("uuid-abc")` is called after `insertBatch("uuid-abc", "20260723,20260724")`
- **THEN** it returns true

### Requirement: FileManager atomic file operations

The system SHALL perform atomic writes using a `.tmp` → `rename` pattern, and recover orphaned `.tmp` files on startup.

#### Scenario: Atomic write via tmp file

- **WHEN** `FileManager::writeDailyFile()` is called
- **THEN** content is first written to a `.tmp` file, then renamed to the target `.txt` file

#### Scenario: Startup recovery of tmp files

- **WHEN** `FileManager::recoverTmpFiles()` is called and `data/2026/07/23.txt.tmp` exists
- **THEN** the `.tmp` file is renamed to `data/2026/07/23.txt`

#### Scenario: Parse daily file with time sorting

- **WHEN** `readDailyFile()` is called and the file contains records at "10:30" and "09:54"
- **THEN** returned records are sorted with "09:54" before "10:30"

#### Scenario: Serialize records with proper separators

- **WHEN** `serializeContent()` is called with two DailyRecord items
- **THEN** output format is "HH:MM\ncontent\n\nHH:MM\ncontent" with records separated by double newlines

### Requirement: DataManager facade coordination

The system SHALL provide a DataManager class that coordinates FileManager and DatabaseManager, serving as the single data access point for upper layers.

#### Scenario: Add record with optimistic lock

- **WHEN** `DataManager::addRecord(2026, 7, 23, "14:00", "修复bug")` is called
- **THEN** the record is appended to the daily file, sorted, written atomically, and index is updated with version check

#### Scenario: Merge records with batch tracking

- **WHEN** `DataManager::mergeRecords(recordsByDate, "batch-uuid")` is called
- **THEN** records are merged per date, batch is recorded in `pc_processed_batches`, and affected index entries are updated

#### Scenario: Version conflict returns error

- **WHEN** a concurrent modification causes version mismatch during `addRecord` or `mergeRecords`
- **THEN** `ErrorCode::VersionConflict` is returned and no data is modified

### Requirement: ConnectionStateMachine state transitions

The system SHALL maintain connection state with the following valid transitions: Unstarted→Disconnected (start), Disconnected→Connected (on connect), Connected→Syncing (on sync start), Syncing→Connected (on sync complete), Connected→Disconnected (on heartbeat timeout), any→Unstarted (stop).

#### Scenario: Normal connection flow

- **WHEN** `start()` then `onConnect()` is called
- **THEN** state transitions: Unstarted → Disconnected → Connected

#### Scenario: Heartbeat timeout disconnects

- **WHEN** 45 seconds pass without `onHeartbeatReceived()` being called while in Connected state
- **THEN** state transitions to Disconnected and `stateChanged` signal is emitted

#### Scenario: Heartbeat resets timer

- **WHEN** `onHeartbeatReceived()` is called while in Connected state
- **THEN** the 45-second timer is reset

### Requirement: AppCore lifecycle management

The system SHALL provide a singleton AppCore that initializes all core modules in correct order (LogManager → ConfigManager → DatabaseManager → FileManager recovery → DataManager → SyncService → ConnectionStateMachine → HttpServer) and emits `startupCompleted()` on success.

#### Scenario: Successful startup sequence

- **WHEN** `AppCore::init()` is called
- **THEN** all modules are initialized in dependency order and `startupCompleted()` signal is emitted

#### Scenario: Startup failure is reported

- **WHEN** any module initialization fails during `AppCore::init()`
- **THEN** `startupFailed(QString reason)` signal is emitted with a descriptive error message
