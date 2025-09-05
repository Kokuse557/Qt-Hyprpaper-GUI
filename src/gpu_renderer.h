#pragma once
#include <QWidget>
#include <QTimer>
#include <QList>
#include <QRect>
#include "cachedimage.h"

extern int THUMB_HEIGHT;

class QHppQ_GPU : public QWidget {
    Q_OBJECT
public:
    explicit QHppQ_GPU(const QString &cacheFolder, const QString &mainFolder, QWidget *parent = nullptr);

    void loadPixmaps(const QList<CachedImage> &pixs);
    QString currentMonitor() const { return m_currentMonitor; }
    void setCurrentMonitor(const QString &monitor) { m_currentMonitor = monitor; }

    int getThumbnailIndexAtY(int y);
    int getYPositionOfThumbnail(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QList<CachedImage> m_pixmaps;
    QString m_cacheFolder;
    QString m_mainFolder;
    QString m_currentMonitor;

    // Hover / click tracking
    bool m_hovering = false;
    CachedImage m_hoveredImage;
    QRect m_hoveredRect;
    QList<QRect> m_thumbnailRects;
    int m_clickedIndex = -1;
    qreal m_clickFlashProgress = 0.0;
    QTimer hoverTimer;

    void startHoverTimer();
    void stopHoverTimer();
    void drawRow(QPainter &painter, const QList<CachedImage> &rowPixmaps, int y, int rowWidth, int rowHeight);
};
