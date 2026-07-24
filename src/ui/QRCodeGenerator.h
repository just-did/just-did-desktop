#pragma once

#include "qrcodegen.hpp"
#include <QImage>
#include <QPainter>
#include <QString>

// QR Code generator wrapper using Nayuki C++ QR library v1.8.0
class QRCodeGenerator {
public:
    static QImage generate(const QString &text, int size = 400)
    {
        QByteArray textBytes = text.toUtf8();

        try {
            qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
                textBytes.constData(), qrcodegen::QrCode::Ecc::MEDIUM);

            int side = qr.getSize();

            // Render
            int border = 4;
            int totalModules = side + 2 * border;
            int moduleSize = size / totalModules;
            if (moduleSize < 1) moduleSize = 1;
            int imgSize = totalModules * moduleSize;

            QImage img(imgSize, imgSize, QImage::Format_Mono);
            img.fill(1); // white
            QPainter p(&img);
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);

            for (int r = 0; r < side; r++) {
                for (int c = 0; c < side; c++) {
                    if (qr.getModule(c, r)) {
                        p.fillRect((c + border) * moduleSize, (r + border) * moduleSize,
                                   moduleSize, moduleSize, Qt::black);
                    }
                }
            }
            p.end();
            return img;
        } catch (const std::exception &) {
            return {}; // Return null image on failure
        }
    }
};
