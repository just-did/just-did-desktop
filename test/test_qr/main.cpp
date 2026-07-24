/*
 * QR Code generation test using Nayuki C++ library
 *
 * Self-contained unit test verifying QR generation feasibility.
 * Build: cmake -B build && cmake --build build
 */

#include "qrcodegen.hpp"

#include <QImage>
#include <QPainter>
#include <QFile>
#include <QDebug>

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        g_failed++; \
    } else { \
        g_passed++; \
    } \
} while(0)

// ---------------------------------------------------------------------------
// Finder pattern: 7x7 fixed pattern at a given corner
// Corner coordinates are (rx, ry) — the top-left module of the 7x7 block.
// ---------------------------------------------------------------------------

static bool checkFinderAt(const qrcodegen::QrCode &qr, int rx, int ry) {
    int size = qr.getSize();
    if (rx + 7 > size || ry + 7 > size)
        return false;

    // Outer border (all black)
    for (int i = 0; i < 7; i++) {
        if (!qr.getModule(rx + i, ry))     return false; // top row
        if (!qr.getModule(rx + i, ry + 6)) return false; // bottom row
        if (!qr.getModule(rx, ry + i))     return false; // left col
        if (!qr.getModule(rx + 6, ry + i)) return false; // right col
    }

    // Inner white ring (positions 1..5 of row 1, col 1, row 5, col 5)
    for (int i = 1; i <= 5; i++) {
        if (qr.getModule(rx + i, ry + 1))     return false;
        if (qr.getModule(rx + i, ry + 5))     return false;
        if (qr.getModule(rx + 1, ry + i))     return false;
        if (qr.getModule(rx + 5, ry + i))     return false;
    }

    // Inner 3x3 black square at (2,2)
    for (int r = 2; r <= 4; r++)
        for (int c = 2; c <= 4; c++)
            if (!qr.getModule(rx + r, ry + c))
                return false;

    // Separator: row 7 and col 7 should be all white (if within bounds)
    if (rx + 7 < size)
        for (int i = 0; i < 7; i++)
            if (qr.getModule(rx + 7, ry + i)) return false;
    if (ry + 7 < size)
        for (int i = 0; i < 7; i++)
            if (qr.getModule(rx + i, ry + 7)) return false;

    return true;
}

// ---------------------------------------------------------------------------
// Module diversity: both black and white modules exist
// ---------------------------------------------------------------------------

static bool hasBothColors(const qrcodegen::QrCode &qr) {
    int size = qr.getSize();
    bool hasBlack = false, hasWhite = false;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y))
                hasBlack = true;
            else
                hasWhite = true;
            if (hasBlack && hasWhite) return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Render QR modules to QImage and save PNG
// ---------------------------------------------------------------------------

static QImage renderToImage(const qrcodegen::QrCode &qr, int border, int moduleSize) {
    int side = qr.getSize();
    int totalModules = side + 2 * border;
    int imgSize = totalModules * moduleSize;

    QImage img(imgSize, imgSize, QImage::Format_Mono);
    img.fill(1); // white

    QPainter p(&img);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);

    for (int y = 0; y < side; y++) {
        for (int x = 0; x < side; x++) {
            if (qr.getModule(x, y)) {
                p.fillRect((x + border) * moduleSize,
                           (y + border) * moduleSize,
                           moduleSize, moduleSize,
                           Qt::black);
            }
        }
    }
    p.end();
    return img;
}

// ---------------------------------------------------------------------------
// Print QR code as ASCII to stdout
// ---------------------------------------------------------------------------

static void printAscii(const qrcodegen::QrCode &qr) {
    int size = qr.getSize();
    printf("\nQR Code (version implied by size=%d):\n\n", size);
    // Top border
    for (int c = 0; c < size + 2; c++) printf("██");
    printf("\n");
    for (int y = 0; y < size; y++) {
        printf("██"); // left border
        for (int x = 0; x < size; x++) {
            printf("%s", qr.getModule(x, y) ? "██" : "  ");
        }
        printf("██"); // right border
        printf("\n");
    }
    for (int c = 0; c < size + 2; c++) printf("██");
    printf("\n");
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

static void testBasicUrl() {
    printf("[TEST] Short URL\n");
    const char *url = "http://192.168.1.1:18080";

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url, qrcodegen::QrCode::Ecc::MEDIUM);
    int size = qr.getSize();
    printf("  Version implied size: %d\n", size);

    CHECK(size >= 21, "QR code size >= 21 (version >= 1)");
    CHECK(hasBothColors(qr), "Module matrix has both black and white modules");

    // Finder patterns at three corners
    CHECK(checkFinderAt(qr, 0, 0), "Finder pattern at top-left");
    CHECK(checkFinderAt(qr, size - 7, 0), "Finder pattern at top-right");
    CHECK(checkFinderAt(qr, 0, size - 7), "Finder pattern at bottom-left");

    // Render and save
    QImage img = renderToImage(qr, 4, 8);
    CHECK(!img.isNull(), "QImage render produced non-null image");
    CHECK(img.width() == (size + 8) * 8, "QImage has correct width");
    CHECK(img.height() == (size + 8) * 8, "QImage has correct height");

    bool saved = img.save("qr_test_output.png");
    CHECK(saved, "PNG saved successfully");

    printAscii(qr);
}

static void testEmptyString() {
    printf("[TEST] Empty string\n");

    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText("", qrcodegen::QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        printf("  Version implied size: %d\n", size);
        CHECK(size >= 21, "Empty string QR code produced with valid size");
        CHECK(checkFinderAt(qr, 0, 0), "Finder pattern at top-left (empty string)");
    } catch (const std::exception &e) {
        fprintf(stderr, "  FAIL: Exception on empty string: %s\n", e.what());
        g_failed++;
    }
}

static void testLongUrl() {
    printf("[TEST] Long URL (~100 chars)\n");

    // Construct a long-ish URL
    std::string url = "http://192.168.1.100:18080/sync/submit?device=phone&batch=abc123def456"
                      "&timestamp=20260724T120000&seq=42&token=xyz";

    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        printf("  URL length: %zu, version implied size: %d\n", url.size(), size);
        CHECK(size >= 21, "Long URL QR code produced with valid size");
        CHECK(hasBothColors(qr), "Long URL QR has both module colors");
        CHECK(checkFinderAt(qr, 0, 0), "Finder at TL (long URL)");
        CHECK(checkFinderAt(qr, size - 7, 0), "Finder at TR (long URL)");
        CHECK(checkFinderAt(qr, 0, size - 7), "Finder at BL (long URL)");
    } catch (const std::exception &e) {
        fprintf(stderr, "  FAIL: Exception on long URL: %s\n", e.what());
        g_failed++;
    }
}

static void testSameUrlProducesSameOutput() {
    printf("[TEST] Same URL produces same output\n");

    const char *url = "http://192.168.1.1:18080";
    qrcodegen::QrCode qr1 = qrcodegen::QrCode::encodeText(url, qrcodegen::QrCode::Ecc::MEDIUM);
    qrcodegen::QrCode qr2 = qrcodegen::QrCode::encodeText(url, qrcodegen::QrCode::Ecc::MEDIUM);

    bool identical = true;
    int size = qr1.getSize();
    CHECK(size == qr2.getSize(), "Same URL: sizes match");
    for (int y = 0; y < size && identical; y++)
        for (int x = 0; x < size && identical; x++)
            if (qr1.getModule(x, y) != qr2.getModule(x, y))
                identical = false;
    CHECK(identical, "Same URL: all modules identical (deterministic)");
}

static void testDifferentUrlsProduceDifferentOutput() {
    printf("[TEST] Different URLs produce different output\n");

    const char *url1 = "http://192.168.1.1:18080";
    const char *url2 = "http://192.168.1.2:18080";

    qrcodegen::QrCode qr1 = qrcodegen::QrCode::encodeText(url1, qrcodegen::QrCode::Ecc::MEDIUM);
    qrcodegen::QrCode qr2 = qrcodegen::QrCode::encodeText(url2, qrcodegen::QrCode::Ecc::MEDIUM);

    int size = qr1.getSize();
    if (size != qr2.getSize()) {
        CHECK(true, "Different URLs: sizes differ (trivially different)");
        return;
    }

    bool anyDifferent = false;
    for (int y = 0; y < size && !anyDifferent; y++)
        for (int x = 0; x < size && !anyDifferent; x++)
            if (qr1.getModule(x, y) != qr2.getModule(x, y))
                anyDifferent = true;
    CHECK(anyDifferent, "Different URLs: at least one module differs");
}

static void testQImageSizes() {
    printf("[TEST] QImage renders at different sizes\n");

    const char *url = "http://192.168.1.1:18080";
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url, qrcodegen::QrCode::Ecc::MEDIUM);
    int side = qr.getSize();

    // moduleSize=4
    QImage img4 = renderToImage(qr, 4, 4);
    CHECK(img4.width() == (side + 8) * 4, "moduleSize=4 correct width");

    // moduleSize=10
    QImage img10 = renderToImage(qr, 4, 10);
    CHECK(img10.width() == (side + 8) * 10, "moduleSize=10 correct width");

    // Different sizes
    CHECK(img4.width() != img10.width(), "Different moduleSize gives different image size");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    printf("=== QR Code Generation Test (Nayuki C++ v1.8.0) ===\n\n");

    testBasicUrl();
    testEmptyString();
    testLongUrl();
    testSameUrlProducesSameOutput();
    testDifferentUrlsProduceDifferentOutput();
    testQImageSizes();

    printf("\n=== Results: %d passed, %d failed ===\n", g_passed, g_failed);
    return g_failed > 0 ? 1 : 0;
}
