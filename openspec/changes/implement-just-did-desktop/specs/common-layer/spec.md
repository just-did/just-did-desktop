## ADDED Requirements

### Requirement: DailyRecord data structure

The system SHALL define `DailyRecord` as a struct containing `time` (QString, "HH:MM" format) and `content` (QString, record body).

#### Scenario: Create a daily record

- **WHEN** code constructs `DailyRecord{"09:54", "完成需求文档初稿"}`
- **THEN** `time` equals "09:54" and `content` equals "完成需求文档初稿"

### Requirement: IndexEntry data structure

The system SHALL define `IndexEntry` as a struct containing year, month, day, path, fileSize, and version fields, matching the `pc_daliy_report_index` SQLite table schema.

#### Scenario: Index entry contains all required fields

- **WHEN** code constructs an IndexEntry with all six fields
- **THEN** all fields are accessible and have correct types (int/int/int/QString/qint64/int)

### Requirement: BatchInfo data structure

The system SHALL define `BatchInfo` as a struct containing batchId (QString) and dates (QStringList).

#### Scenario: Batch info stores multiple affected dates

- **WHEN** code constructs `BatchInfo{"uuid-123", {"20260723", "20260724"}}`
- **THEN** `batchId` equals "uuid-123" and `dates` contains two date strings

### Requirement: ConnectionState enum

The system SHALL define `ConnectionState` as an enum class with four values: Unstarted, Disconnected, Connected, Syncing.

#### Scenario: State values are distinct

- **WHEN** code compares `ConnectionState::Unstarted` against `ConnectionState::Connected`
- **THEN** they are not equal

### Requirement: Constants definitions

The system SHALL define all configuration constants in `Constants.h` under the `Constants` namespace, including default port (18080), heartbeat interval (15s), heartbeat timeout (45s), max fetch files (32), max fetch day span (31), log file max size (4MB), and log total max size (12MB).

#### Scenario: Default port is 18080

- **WHEN** code references `Constants::DEFAULT_PORT`
- **THEN** the value equals 18080

#### Scenario: Heartbeat timeout is 45 seconds

- **WHEN** code references `Constants::HEARTBEAT_TIMEOUT_SEC`
- **THEN** the value equals 45

### Requirement: ErrorCode enum

The system SHALL define `ErrorCode` as an enum class with values: Success=0, StorageError=1, InternalError=-1, TooManyFiles=-2, InvalidParameter=-3, VersionConflict=-4.

#### Scenario: Error code values match API spec

- **WHEN** code returns `ErrorCode::VersionConflict`
- **THEN** the integer value equals -4

### Requirement: Common layer has no behavior logic

The common layer SHALL contain only struct, enum, and constexpr definitions. No methods, I/O, or business logic.

#### Scenario: DailyRecord has no methods

- **WHEN** reviewing `Types.h`
- **THEN** no struct contains member functions beyond possibly a constructor
