## Context

当前 `src/ui/QRCodeGenerator.h` 使用手工裁剪的 Nayuki C 版库（`qrcodegen.c`/`.h`），存在两个问题：
1. `temp[1]` 栈缓冲区溢出（API 要求 `side × side` 字节）
2. `qrcodegen.c` 未编入 `src/ui/CMakeLists.txt`，导致链接失败

`test/test_qr/` 目录中已验证了 Nayuki C++ v1.8.0 方案的正确性，生成 `QImage` 渲染逻辑与主项目一致，可直接迁移。

## Goals / Non-Goals

**Goals:**
- 用 Nayuki C++ 库替换 C 版，消除 buffer 管理 bug
- `QRCodeGenerator::generate()` 对外签名和行为不变
- `QRCodeProvider` 和 QML 层零改动
- 清理旧 C 版文件和废弃测试代码

**Non-Goals:**
- 不改变 QRCodeProvider 的实现或接口
- 不修改 QML 文件
- 不改动 `test/test_qr/` 目录

## Decisions

### 1. 替换策略：最小改动，只改 QRCodeGenerator.h 内部实现

C API：
```c
uint8_t *qrcode = malloc(side * side);  // 手动分配
uint8_t temp[1] = {0};                   // 🔴 bug: 只分配 1 字节
qrcodegen_encodeText(data, len, temp, qrcode, ecl, ver, ver, mask, false);
// 遍历 qrcode[y * side + x] & 1 读模块
free(qrcode);
```

C++ API（替换后）：
```cpp
QrCode qr = QrCode::encodeText(text, Ecc::MEDIUM);  // RAII，无 buffer 管理
int side = qr.getSize();
// qr.getModule(x, y) 读模块
```

**差异**：
- C 版返回 `uint8_t`（0/1 位掩码），C++ 版返回 `bool`（true=黑）
- C 版用 `qrcode[y * side + x] & 1`，C++ 版用 `qr.getModule(x, y)`
- C 版返回 `false` 表示失败并以空 QImage 返回，C++ 版抛 `std::exception`

### 2. 异常处理

`QrCode::encodeText()` 在数据过长时抛 `data_too_long` 异常。`QRCodeGenerator::generate()` 内部 catch 后返回空 `QImage`，行为与旧版失败返回一致。

### 3. 文件操作

- 从 `test/test_qr/` 拷贝 `qrcodegen.hpp` + `qrcodegen.cpp` 到 `src/ui/`
- 删除 `src/ui/qrcodegen.c`、`src/ui/qrcodegen.h`
- 删除 `src/ui/test_qrcode.cpp`、`build_cmd.bat`、`build_qr_test.bat`

## Risks / Trade-offs

- **C++ 异常 vs C 返回值**：C++ 版失败时抛异常，需 catch 处理 → 在 `generate()` 中 try-catch
- **编译依赖**：`qrcodegen.cpp` 仅依赖 C++ 标准库，与 Qt 无冲突

## Open Questions

无
