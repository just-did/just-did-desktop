## Context

项目 QR 码生成功能依赖手工裁剪的 Nayuki qrcodegen C 版本，存在 `temp` 缓冲区栈溢出 bug。Nayuki 官方提供了完整的 C++ 版本，采用 RAII 内存管理，API 更干净。需要在 `test/test_qr/` 搭建独立实验环境，将 C++ 版源文件直接加入测试目录，验证方案可行性。

## Goals / Non-Goals

**Goals:**
- 将 Nayuki C++ 版 6 个源文件（`QrCode.hpp/.cpp`、`QrSegment.hpp/.cpp`、`BitBuffer.hpp/.cpp`）直接放入 `test/test_qr/`
- `main.cpp` 调用 `qrcodegen::QrCode::encodeText()` 生成 QR 码
- 验证 finder pattern、模块多样性、QImage 渲染 + PNG 保存
- 清理所有 Python 脚本和历史输出产物

**Non-Goals:**
- 不修改 `src/` 下任何文件
- 不修复项目中的 QR bug
- 不引入 CMake FetchContent 或第三方包管理器

## Decisions

### 1. 源文件方式：直接放入而非 FetchContent

**选择**：从 Nayuki v1.8.0 的 `cpp/` 目录拷贝 6 个源文件到 `test/test_qr/`。

**原因**：
- 纯离线，无需网络，无需 CMake FetchContent 配置
- 测试目录完全自包含
- 简单直接，后期迁移到主项目时同样只需拷这 6 个文件

**文件清单**（实际只有 2 个源文件，并非 6 个）：
```
test/test_qr/
  ├── qrcodegen.hpp              ← 所有类（QrCode, QrSegment, BitBuffer）的声明
  ├── qrcodegen.cpp              ← 全部实现
  ├── main.cpp                   ← 新建：测试入口
  └── CMakeLists.txt             ← 更新
```

### 2. API：C++ 版 vs C 版

| | C 版（当前项目） | C++ 版（本次引入） |
|---|---|---|
| 头文件 | `qrcodegen.h`（手工裁剪） | `QrCode.hpp`（官方完整版） |
| 生成调用 | `qrcodegen_encodeText(data, len, temp, qrcode, ecl, ...)` | `QrCode::encodeText(text, Ecc::MEDIUM)` |
| 内存 | 手动分配 qrcode + temp 数组 | RAII，构造函数自动分配 |
| 读模块 | `qrcode[y * side + x] & 1` | `qr.getModule(x, y)` |

### 3. 渲染输出：QImage + QPainter

```cpp
QImage img(imgSize, imgSize, QImage::Format_Mono);
img.fill(1);   // 白色背景
QPainter p(&img);
// 遍历 getModule(x, y)，黑色模块 fillRect
```

与主项目 `QRCodeGenerator.h` 中的渲染逻辑完全一致，保证迁移时零差异。

## Risks / Trade-offs

- **库文件需自行拷贝**：6 个源文件需手动从 Nayuki 仓库获取。→ 影响极小，一次性操作，且文件体积小（总计 ~80KB）。
- **与主项目 C 版不冲突**：C++ 版使用不同命名空间 `qrcodegen::`，可以和项目中已有的 C 版共存。

## Open Questions

无
