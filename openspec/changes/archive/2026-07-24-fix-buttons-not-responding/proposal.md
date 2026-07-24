## Why

启动应用后，主窗口上所有按钮（日历翻页、HTTP 服务启动/停止、日报提交等）均无法响应鼠标点击。根因是 HEAD 提交 `b0111b9` 引入的 `QQuickWindow::setDefaultAlphaBuffer(true)` 在 Windows 上与 `Qt.FramelessWindowHint` 组合时，导致 Window DWM 无法正确进行 hit-testing，所有鼠标事件穿透窗口。

## What Changes

- 删除 `src/main.cpp` 中的 `QQuickWindow::setDefaultAlphaBuffer(true)` 调用
- 删除不再需要的 `#include <QDateTime>`（若仅为此功能引入）

## Capabilities

### New Capabilities

<!-- None — this is a bug fix restoring existing behavior -->

### Modified Capabilities

<!-- None — no spec-level requirement changes -->

## Impact

- [src/main.cpp](src/main.cpp): 删除 `setDefaultAlphaBuffer(true)` 及相关头文件引用
- 无需修改 QML、ViewModels 或其他模块
