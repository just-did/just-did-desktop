## Why

`src/ui/QRCodeGenerator.h` 使用手工裁剪的 Nayuki qrcodegen C 版本，存在 `temp[1]` 栈缓冲区溢出 bug（API 要求 `side×side` 字节却只分配 1 字节），且 C 版源文件未编入 CMakeLists.txt 导致无法链接。已在 `test/test_qr/` 中使用 Nayuki C++ v1.8.0 验证了正确方案，现将其应用到主项目。

## What Changes

- **删除** `src/ui/qrcodegen.c`、`src/ui/qrcodegen.h`：手工裁剪的 C 版 QR 库
- **删除** `src/ui/test_qrcode.cpp`、`build_cmd.bat`、`build_qr_test.bat`：旧命令行测试，已被 `test/test_qr/` 替代
- **新增** `src/ui/qrcodegen.hpp`、`src/ui/qrcodegen.cpp`：Nayuki C++ v1.8.0（从 `test/test_qr/` 拷贝）
- **重写** `src/ui/QRCodeGenerator.h`：用 `qrcodegen::QrCode::encodeText()` 替换 C API，消除 buffer 管理问题
- **更新** `src/ui/CMakeLists.txt`：添加 `qrcodegen.cpp` 编译

## Capabilities

### New Capabilities
- `qr-code-gen`: 主项目 QR 码生成模块，使用 Nayuki C++ 库 `QrCode::encodeText()` 生成 QR 码模块矩阵并渲染为 QImage

### Modified Capabilities
<!-- 无现有 capability 被修改 -->

## Impact

- 影响目录：`src/ui/`、项目根目录（清理 2 个 `.bat`）
- 对外接口不变：`QRCodeGenerator::generate(text, size)` 签名保持，`QRCodeProvider` 和 QML 层零改动
- 无构建依赖变化：仅将 `.c` 替换为 `.cpp`
