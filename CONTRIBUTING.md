# 贡献指南

感谢你考虑为「刚刚做了什么」电脑端贡献代码！请先阅读 [行为准则](CODE_OF_CONDUCT.md)，我们期望所有参与者共同维护一个友善、包容的社区。

## 报告问题

- 报告 Bug 或提出功能建议请使用对应的 Issue 模板，尽量填写完整信息
- Bug 报告请附上**复现步骤**，以及 `qt_debug.log`（exe 同目录）或 `logs/just-did.log` 中的相关片段——日志只记录程序运行信息，**不含任何日报内容**，可放心粘贴
- 提交前先搜索一下，确认没有相同的 Issue

## 开发环境

见 [README 的构建章节](README.md#构建)：Windows + Visual Studio 2022 + Qt 6.8 msvc2022_64 套件 + CMake 3.20+，第三方依赖由 FetchContent 自动拉取。

## 代码结构

| 目录 | 库 | 职责 |
|------|------|------|
| `src/common/` | justdid_common（纯头文件） | `Types.h` 数据结构、`ErrorCode.h` 错误码枚举、`Constants.h` 常量 |
| `src/core/` | justdid_core | 配置、滚动日志、SQLite、文件读写、业务逻辑（原子写入 + 乐观锁） |
| `src/service/` | justdid_service | HTTP 服务路由、同步状态机（暂存/覆盖）、日报查询与清理封装 |
| `src/ui/` | justdid_ui | QML 界面、ViewModels、Models、二维码生成 |

依赖方向严格为 `ui → service → core → common`，禁止反向依赖。`main.cpp` 是组合根，负责创建服务、注入 ViewModel 并注册到 QML 上下文。

## 代码风格

- C++17；类与方法 PascalCase，成员变量 `m` 前缀（如 `mPort`）
- 业务逻辑放 ViewModel/Service 层，QML 只做展示与交互
- 错误码统一使用 `src/common/ErrorCode.h` 中的枚举，禁止裸数字
- 注释建议用英文（便于开源协作）；资源文件（如 `src/app.rc`）中的字符串必须为 ASCII
- 提交信息格式：`type: 描述`（`feat` / `fix` / `chore` / `refactor` / `docs` / `test`），与现有历史风格一致
- **严禁提交运行时数据**：`data/`、`just_do.db`、`config.yml`、`qr_debug.png` 等已在 `.gitignore` 中，PR 提交前请自查

## 测试

### test_qr（C++，QR 码生成单元测试）

独立 CMake 工程，不依赖主工程：

```bash
cmake -B test/test_qr/build -S test/test_qr -DCMAKE_PREFIX_PATH="<你的 Qt msvc2022_64 路径>"
cmake --build test/test_qr/build --config Release
./test/test_qr/build/Release/QRCodeTest.exe   # 退出码 0 即通过（需要 Qt bin 在 PATH）
```

### Python 集成测试（需先启动应用）

三个脚本均使用 Python 3 标准库，**先启动 JustDid.exe**（HTTP 服务默认端口 18080），再执行：

```bash
python test/test_submit.py              # /sync/submit 全场景（正常/幂等/非法参数/断点续跑等）
python test/test_fetch/test_fetch.py           # /sync/fetch 拉取
python test/test_fetch_index/test_fetch_index.py  # /sync/fetch-index 索引拉取
```

可用 `--scenario <名称>` 跑单个场景、`--app-root <路径>` 指定应用根目录（详见各脚本头部 docstring）。**注意：集成测试会向 `data/` 与 `just_do.db` 写入测试日期（2026-08-01 ~ 03）的数据。**

## Pull Request 流程

1. Fork 本仓库，从 `main` 分支创建功能分支
2. 修改代码，补充或更新相关测试
3. 本地 Release 构建通过（见 README 构建章节）
4. 提交（自查：没有混入运行时数据文件）
5. 发起 PR，使用 PR 模板填写变更摘要与测试情况
6. CI 必须通过；维护者 Review 后合并

## 许可证

向本项目提交贡献即表示你同意在 [Apache License 2.0](LICENSE) 条款下授权你的贡献。
