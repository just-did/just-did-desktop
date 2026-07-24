# qr-code-gen

## Purpose

主项目 QR 码生成模块，使用 Nayuki C++ 库生成二维码模块矩阵并渲染为 QImage，供 QML ImageProvider 使用。

## Requirements

### Requirement: QR Code Generation via Nayuki C++ API
The system SHALL generate QR code images using `qrcodegen::QrCode::encodeText()` from the Nayuki C++ library.

#### Scenario: Generate QR code from URL text
- **WHEN** `QRCodeGenerator::generate(url, size)` is called with a valid URL
- **THEN** a non-null `QImage` is returned with dimensions `(side + 8) * moduleSize`
- **AND** the QImage contains a valid QR code with finder patterns at three corners

#### Scenario: Generate fails gracefully on invalid input
- **WHEN** `QRCodeGenerator::generate()` receives input that causes `encodeText()` to throw
- **THEN** the exception is caught and a null `QImage` is returned

### Requirement: Build System Includes Nayuki C++ Source
The `justdid_ui` library SHALL compile `qrcodegen.cpp` and include `qrcodegen.hpp`.

#### Scenario: Project builds with C++ library
- **WHEN** the project is built via `cmake --build build`
- **THEN** `qrcodegen.cpp` compiles and links successfully into `justdid_ui`
- **AND** no C version files (`qrcodegen.c`, `qrcodegen.h`) remain in `src/ui/`

### Requirement: Backward Compatible Interface
The `QRCodeGenerator::generate()` static method SHALL maintain the same signature: `static QImage generate(const QString &text, int size = 400)`.

#### Scenario: QRCodeProvider works without changes
- **WHEN** `QRCodeProvider::generateQRCode(url)` is called
- **THEN** it returns a valid `QImage` through the same `QRCodeGenerator::generate()` call path
- **AND** existing QML code using `image://qrcode/current` continues to work unchanged
