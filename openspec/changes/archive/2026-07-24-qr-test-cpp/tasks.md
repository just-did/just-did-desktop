## 1. 清理测试目录

- [x] 1.1 删除 Python 脚本：`gen_qr.py`、`dump_modules.py`
- [x] 1.2 删除历史输出产物：`qr_c.png`、`qr_python.png`、`qr_test_output.png`、`qr_test_ascii.txt`
- [x] 1.3 删除 C 版本地副本：`qrcodegen.c`、`qrcodegen.h`

## 2. 添加 Nayuki C++ 源文件

- [x] 2.1 从 Nayuki v1.8.0 的 `cpp/` 目录拷贝源文件到 `test/test_qr/`：`qrcodegen.hpp`、`qrcodegen.cpp`

## 3. 编写 C++ 测试代码

- [x] 3.1 新建 `test/test_qr/main.cpp`，include `qrcodegen.hpp`
- [x] 3.2 实现 finder pattern 校验函数
- [x] 3.3 实现模块矩阵 → `QImage(Format_Mono)` → `QImage::save()` PNG 渲染
- [x] 3.4 编写测试用例：空字符串、短 URL、长 URL
- [x] 3.5 编写测试用例：finder pattern 三个角落校验
- [x] 3.6 编写测试用例：模块多样性校验
- [x] 3.7 输出 ASCII 二维码到 stdout

## 4. 更新构建配置

- [x] 4.1 更新 `test/test_qr/CMakeLists.txt`，添加 qrcodegen.cpp + main.cpp
- [x] 4.2 构建测试程序并验证编译通过
- [x] 4.3 运行测试，验证所有断言通过
- [x] 4.4 用手机扫码验证生成的 PNG 文件
