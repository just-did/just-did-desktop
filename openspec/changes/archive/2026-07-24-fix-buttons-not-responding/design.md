## Context

`QQuickWindow::setDefaultAlphaBuffer(true)` 在 Windows 上与 `Qt.FramelessWindowHint` 组合时，会导致 Windows DWM 无法正确进行 hit-testing，所有鼠标事件穿透窗口，使界面按钮无法响应点击。

已有三重防黑屏闪烁机制（`visible: false`、`setColor("#ffffff")`、`QTimer::singleShot` 延迟显示），不需要 alpha buffer。

## Goals / Non-Goals

**Goals:**
- 恢复主窗口所有按钮的鼠标点击响应能力
- 保持启动时无黑屏闪烁

**Non-Goals:**
- 不改变 UI 布局或交互逻辑
- 不引入新的启动闪烁问题

## Decisions

**Decision: 直接删除 `QQuickWindow::setDefaultAlphaBuffer(true)`**

- 这是最简修复方案，一行代码改动
- 现有的三重防闪烁机制足以防止黑屏出现
- 替代方案（如手动设置 WS_EX_LAYERED + 处理 WM_NCHITTEST）过于复杂且不必要

## Risks / Trade-offs

- [Risk] 移除后可能出现短暂黑屏 → **Mitigation**: `visible: false` + `QTimer::singleShot` 延迟显示 + `setColor("#ffffff")` 三层防护已在代码中
