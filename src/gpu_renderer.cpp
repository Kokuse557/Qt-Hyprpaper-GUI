#include "gpu_renderer.h"
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QFileInfo>
#include <QtMath>
#include <QDateTime>
#include "reload.h"


// int THUMB_HEIGHT = 200;
const int SPACING = 10;
const int FOLDER_GAP = 30;

QHppQ_GPU::QHppQ_GPU(const QString &cacheFolder, const QString &mainFolder, QWidget *parent)
    : QWidget(parent), m_cacheFolder(cacheFolder), m_mainFolder(mainFolder)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
}

void QHppQ_GPU::loadPixmaps(const QList<CachedImage> &pixs) {
    m_pixmaps = pixs;
    update();
}

int QHppQ_GPU::getThumbnailIndexAtY(int y) {
    int rowHeight = THUMB_HEIGHT;
    int currentY = 0;
    int rowWidth = 0;
    QString currentFolder;
    QList<CachedImage> rowPixmaps;

    for (int i = 0; i < m_pixmaps.size(); ++i) {
        const CachedImage &cimg = m_pixmaps[i];
        int w = cimg.pix.width() * rowHeight / cimg.pix.height();

        if (!currentFolder.isEmpty() && currentFolder != cimg.folder) {
            currentY += rowHeight + FOLDER_GAP;
            rowPixmaps.clear();
            rowWidth = 0;
        }
        currentFolder = cimg.folder;

        if (rowWidth + w + (rowPixmaps.size() > 0 ? SPACING : 0) > width()) {
            currentY += rowHeight + SPACING;
            rowPixmaps.clear();
            rowWidth = 0;
        }

        if (y >= currentY && y < currentY + rowHeight)
            return i;

        rowPixmaps.append(cimg);
        rowWidth += w + (rowPixmaps.size() > 0 ? SPACING : 0);
    }
    return m_pixmaps.size() - 1;
}

int QHppQ_GPU::getYPositionOfThumbnail(int index) {
    int rowHeight = THUMB_HEIGHT;
    int currentY = 0;
    int rowWidth = 0;
    QString currentFolder;
    QList<CachedImage> rowPixmaps;

    for (int i = 0; i <= index && i < m_pixmaps.size(); ++i) {
        const CachedImage &cimg = m_pixmaps[i];
        int w = cimg.pix.width() * rowHeight / cimg.pix.height();

        if (!currentFolder.isEmpty() && currentFolder != cimg.folder) {
            currentY += rowHeight + FOLDER_GAP;
            rowPixmaps.clear();
            rowWidth = 0;
        }
        currentFolder = cimg.folder;

        if (rowWidth + w + (rowPixmaps.size() > 0 ? SPACING : 0) > width()) {
            currentY += rowHeight + SPACING;
            rowPixmaps.clear();
            rowWidth = 0;
        }

        rowPixmaps.append(cimg);
        rowWidth += w + (rowPixmaps.size() > 0 ? SPACING : 0);

        if (i == index) return currentY;
    }
    return 0;
}

void QHppQ_GPU::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int y = 0;
    int rowHeight = THUMB_HEIGHT;
    QString currentFolder;
    QList<CachedImage> rowPixmaps;
    int rowWidth = 0;

    m_thumbnailRects.clear();

    for (const CachedImage &cimg : m_pixmaps) {
        int w = cimg.pix.width() * rowHeight / cimg.pix.height();

        if (!currentFolder.isEmpty() && currentFolder != cimg.folder) {
            drawRow(painter, rowPixmaps, y, rowWidth, rowHeight);
            y += rowHeight + FOLDER_GAP;
            rowPixmaps.clear();
            rowWidth = 0;
        }
        currentFolder = cimg.folder;

        if (rowWidth + w + (rowPixmaps.size() > 0 ? SPACING : 0) > width()) {
            drawRow(painter, rowPixmaps, y, rowWidth, rowHeight);
            y += rowHeight + SPACING;
            rowPixmaps.clear();
            rowWidth = 0;
        }

        rowPixmaps.append(cimg);
        rowWidth += w + (rowPixmaps.size() > 1 ? SPACING : 0);
    }

    if (!rowPixmaps.isEmpty()) {
        drawRow(painter, rowPixmaps, y, rowWidth, rowHeight);
        y += rowHeight + SPACING;
    }

    setMinimumHeight(y);

    if (m_hovering || (m_clickedIndex >= 0 && m_clickFlashProgress > 0.0)) update();
}

void QHppQ_GPU::drawRow(QPainter &painter, const QList<CachedImage> &rowPixmaps, int y, int rowWidth, int rowHeight) {
    int x = (width() - rowWidth) / 2;

    for (const CachedImage &rpix : rowPixmaps) {
        int rw = rpix.pix.width() * rowHeight / rpix.pix.height();
        QRect thumbRect(x, y, rw, rowHeight);

        if (m_hovering && m_hoveredImage.filePath == rpix.filePath) {
            int hoverAlpha = 40 + int(15 * std::sin(QDateTime::currentMSecsSinceEpoch() / 100.0));
            QPixmap bright = rpix.pix;
            QPainter tmp(&bright);
            tmp.fillRect(bright.rect(), QColor(255,255,255, hoverAlpha));
            tmp.end();
            painter.drawPixmap(thumbRect, bright);
        } else {
            painter.setOpacity(0.85);
            painter.drawPixmap(thumbRect, rpix.pix);
            painter.setOpacity(1.0);
        }

        if (m_clickedIndex >= 0 && m_clickedIndex < m_pixmaps.size() &&
            m_pixmaps[m_clickedIndex].filePath == rpix.filePath &&
            m_clickFlashProgress > 0.0)
        {
            painter.fillRect(thumbRect, QColor(255,255,255,int(100*m_clickFlashProgress)));
        }

        m_thumbnailRects.append(thumbRect);
        x += rw + SPACING;
    }
}

void QHppQ_GPU::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void QHppQ_GPU::mousePressEvent(QMouseEvent* event) {
    m_clickedIndex = -1;
    for (int i=0;i<m_pixmaps.size();++i) {
        if (m_thumbnailRects[i].contains(event->pos())) {
            m_clickedIndex = i;
            m_clickFlashProgress = 1.0;
            QTimer::singleShot(150, this, [this](){ m_clickFlashProgress=0.0; update(); });
            update();
            break;
        }
    }

    if (m_clickedIndex >=0 && m_clickedIndex < m_pixmaps.size()) {
        QString filePath = QFileInfo(m_pixmaps[m_clickedIndex].filePath).absoluteFilePath();
        QString monitor = m_currentMonitor;
        recordClick(monitor,filePath);
        updateHyprpaperWallpaper(monitor,filePath);
    }

    QWidget::mousePressEvent(event);
}

void QHppQ_GPU::mouseMoveEvent(QMouseEvent* event) {
    m_hovering = false;
    for (int i=0;i<m_pixmaps.size();++i) {
        if (m_thumbnailRects[i].contains(event->pos())) {
            m_hovering = true;
            m_hoveredImage = m_pixmaps[i];
            m_hoveredRect = m_thumbnailRects[i];
            startHoverTimer();
            break;
        }
    }
    if (!m_hovering) stopHoverTimer();
    update();
    QWidget::mouseMoveEvent(event);
}

void QHppQ_GPU::startHoverTimer() {
    if (!hoverTimer.isActive()) {
        connect(&hoverTimer, &QTimer::timeout, this, [=](){ update(); });
        hoverTimer.start(90);
    }
}

void QHppQ_GPU::stopHoverTimer() {
    if (hoverTimer.isActive()) hoverTimer.stop();
}
