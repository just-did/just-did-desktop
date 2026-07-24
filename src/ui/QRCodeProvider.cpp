#include "QRCodeProvider.h"
#include "QRCodeGenerator.h"
#include "core/LogManager.h"

QImage QRCodeProvider::generateQRCode(const QString &url)
{
    LogManager::instance()->info(QString("[QRCode] 开始生成二维码, URL=%1, 长度=%2").arg(url).arg(url.length()));

    // Generate at high resolution for scan-ability
    QImage result = QRCodeGenerator::generate(url, 400);

    if (result.isNull()) {
        LogManager::instance()->error(QString("[QRCode] 二维码生成失败! URL=%1").arg(url));
    } else {
        LogManager::instance()->info(QString("[QRCode] 二维码生成成功, 尺寸=%1x%2")
                                         .arg(result.width()).arg(result.height()));
        // Save to file for debugging
        result.save("qr_debug.png");
        LogManager::instance()->info("[QRCode] 已保存到 qr_debug.png");
    }
    return result;
}
