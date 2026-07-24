#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

// Image provider that generates QR code images for QML
// Access via "image://qrcode/current" in QML
class QRCodeProvider : public QQuickImageProvider
{
public:
    QRCodeProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    void setUrl(const QString &url) {
        QMutexLocker lock(&mMutex);
        if (mUrl != url) {
            mUrl = url;
            mCachedImage = {}; // Invalidate cache to force re-generation
            mVersion++;
        }
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(id)
        QMutexLocker lock(&mMutex);

        // Add version info to log
        static int requestCount = 0;
        requestCount++;

        if (mCachedImage.isNull() && !mUrl.isEmpty()) {
            mCachedImage = generateQRCode(mUrl);
        }

        if (size) *size = mCachedImage.size();

        if (requestedSize.isValid() && !mCachedImage.isNull()) {
            return mCachedImage.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        return mCachedImage;
    }

    int version() const { return mVersion; }

    static QImage generateQRCode(const QString &url);

private:
    QString mUrl;
    QImage mCachedImage;
    QMutex mMutex;
    int mVersion = 0;
};
