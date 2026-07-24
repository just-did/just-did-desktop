## 1. 清理旧文件

- [x] 1.1 删除 `src/ui/qrcodegen.c`、`src/ui/qrcodegen.h`（C 版本地副本）
- [x] 1.2 删除 `src/ui/test_qrcode.cpp`（旧命令行测试）
- [x] 1.3 删除 `build_cmd.bat`、`build_qr_test.bat`（旧测试构建脚本）

## 2. 添加 Nayuki C++ 源文件

- [x] 2.1 从 `test/test_qr/` 拷贝 `qrcodegen.hpp`、`qrcodegen.cpp` 到 `src/ui/`

## 3. 重写 QRCodeGenerator.h

- [x] 3.1 替换 include：`qrcodegen.h` → `qrcodegen.hpp`
- [x] 3.2 用 `qrcodegen::QrCode::encodeText()` 替换 C API 调用
- [x] 3.3 用 `qr.getSize()` / `qr.getModule(x, y)` 替换手动数组索引
- [x] 3.4 添加 try-catch 处理异常，失败时返回空 QImage
- [x] 3.5 移除手动 malloc/free 和 temp buffer

## 4. 更新构建配置

- [x] 4.1 在 `src/ui/CMakeLists.txt` 中添加 `qrcodegen.cpp`
- [x] 4.2 构建主项目验证编译链接通过
- [x] 4.3 运行项目，验证 QR 码正常生成（检查 `qr_debug.png`）
