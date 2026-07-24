## 1. 修复 Qt Quick Layouts 递归重排

- [x] 1.1 将 `src/ui/qml/MainWindow.qml` 第24行的 `Layout.preferredWidth: parent.width * 0.7` 替换为 `Layout.fillWidth: true`
- [x] 1.2 同样修复 `build/src/Debug/qml/MainWindow.qml`（Debug 构建的 QML 副本）

## 2. 额外清理

- [x] 2.1 删除 `src/main.cpp` 中不必要的 `QQuickWindow::setDefaultAlphaBuffer(true)`
- [x] 2.2 将 `#include <QDateTime>` 替换为 `#include <QDate>`（精确引用）
- [x] 2.3 将日志路径改为 `applicationDirPath() + "/qt_debug.log"`（便于定位）

## 3. 验证

- [x] 3.1 重新编译项目并确认构建成功
- [x] 3.2 启动应用，验证主窗口所有按钮（日历翻页、启动/停止、提交日报）均可正常点击响应
- [x] 3.3 验证启动时无黑屏闪烁
