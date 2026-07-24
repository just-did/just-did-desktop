## ADDED Requirements

### Requirement: QR Code Generation via Nayuki C++ Library
The test SHALL generate QR codes using `qrcodegen::QrCode::encodeText()` from the Nayuki C++ source files placed directly in `test/test_qr/`.

#### Scenario: Short URL generates valid QR code
- **WHEN** the input text is `"http://192.168.1.1:18080"`
- **THEN** `QrCode::encodeText(text, Ecc::MEDIUM)` returns a valid `QrCode` object
- **AND** `qr.getSize()` returns a non-zero module side length
- **AND** the module matrix contains both black and white modules

#### Scenario: Empty string edge case
- **WHEN** the input text is an empty string `""`
- **THEN** `QrCode::encodeText("", Ecc::MEDIUM)` completes without throwing an exception
- **AND** a valid `QrCode` object is returned

### Requirement: Finder Pattern Verification
A valid QR code SHALL contain finder patterns (7×7 black-white-black alternating squares) at top-left, top-right, and bottom-left corners.

#### Scenario: Finder patterns exist on generated QR
- **WHEN** a QR code is generated from a typical URL
- **THEN** the 7×7 finder pattern is present at top-left via `qr.getModule()`
- **AND** the 7×7 finder pattern is present at top-right
- **AND** the 7×7 finder pattern is present at bottom-left

### Requirement: QR Code Renders as QImage
The module matrix SHALL render as `QImage(Format_Mono)` with border=4 and configurable module size.

#### Scenario: Render and save to PNG
- **WHEN** a valid module matrix is rendered with moduleSize=8
- **THEN** the QImage is non-null with size `(size + 2*border) * moduleSize`
- **AND** `QImage::save("qr_test_output.png")` produces a non-empty PNG file

### Requirement: Self-Contained Test Build
The test SHALL build independently from the main project, using only source files within `test/test_qr/`.

#### Scenario: Build succeeds with local sources only
- **WHEN** `cmake -B build && cmake --build build` is executed in `test/test_qr/`
- **THEN** the build succeeds using only local `.cpp`/`.hpp` files and `Qt6::Core + Qt6::Gui`
- **AND** no files from `src/` are included or linked
