## Why

项目中二维码生成功能存在问题（`QRCodeGenerator` 中 `temp` 缓冲区分配过小导致栈溢出），当前使用的是手工裁剪的 Nayuki qrcodegen C 版本。需要在 `test/test_qr` 中搭建一套自包含的 C++ 单元测试环境，用正经的 **Nayuki QR-Code-generator C++ 版**（源码直接加入），验证 QR 生成可行性，确认方法正确后再将修复应用到主项目。

## What Changes

- **删除** `test/test_qr/` 下的 Python 脚本、历史输出产物、以及 C 版 `qrcodegen.c` / `qrcodegen.h` 本地副本
- **新增** Nayuki C++ 版源文件到 `test/test_qr/`：`QrCode.hpp/.cpp`、`QrSegment.hpp/.cpp`、`BitBuffer.hpp/.cpp`（从 [nayuki/QR-Code-generator](https://github.com/nayuki/QR-Code-generator) v1.8.0 的 `cpp/` 目录拷贝）
- **新建** `test/test_qr/main.cpp`：C++ 单元测试，调用 `qrcodegen::QrCode::encodeText()` 生成 QR 码，校验 + QImage 渲染
- **更新** `test/test_qr/CMakeLists.txt`：编译上述源文件 + main.cpp

## Capabilities

### New Capabilities
- `qr-code-test`: 自包含的 QR 码生成单元测试，使用 Nayuki C++ 库源文件，验证 QR 生成正确性

### Modified Capabilities
<!-- 无 -->

## Impact

- 影响目录：`test/test_qr/`（仅测试代码，不影响主项目构建或运行时）
- 不修改 `src/` 下任何文件
- 纯离线构建，无网络依赖
