#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QDirIterator>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSlider>
#include <QDebug>
#include <QCryptographicHash>
#include <QFileSystemWatcher>
#include <QScrollBar>
#include <QTimer>
#include <QMouseEvent>
#include <QSettings>
#include <QProcess>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <QDir>
#include <QString>
#include <QFile>
#include <QTextStream> 


#include "reload.h"
#include "paths.h"
#include "inotifywatcher.h"



int THUMB_HEIGHT = 200; 
const int SPACING = 10;
const int FOLDER_GAP = 30;
const int WINDOW_PADDING = 20;

struct CachedImage {
    QPixmap pix;
    QString folder;
    QString filePath;

    bool operator==(const CachedImage &other) const {
        return filePath == other.filePath; // unique by path
    }
};

class QHppQ : public QWidget {
    Q_OBJECT
public:
    QString currentMonitor() const { return m_currentMonitor; }
    void setCurrentMonitor(const QString &monitor) { m_currentMonitor = monitor; }

    QHppQ(const QString &cacheFolder, const QString &mainFolder, QWidget *parent = nullptr)
        : QWidget(parent), m_cacheFolder(cacheFolder), m_mainFolder(mainFolder)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);

        // real-time inotify-based watcher
        InotifyWatcher *iw = new InotifyWatcher(mainFolder, this);

        connect(iw, &InotifyWatcher::fileCreated, this, [&](const QString &path){
            qDebug() << "Created:" << path;
            loadFilteredPixmaps(m_cacheFolder, m_mainFolder);
            update();
        });

        connect(iw, &InotifyWatcher::fileDeleted, this, [&](const QString &path){
            qDebug() << "Deleted:" << path;
            loadFilteredPixmaps(m_cacheFolder, m_mainFolder);
            update();
        });

        // initial load
        loadFilteredPixmaps(m_cacheFolder, m_mainFolder);
    }

    int getThumbnailIndexAtY(int y) {
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

    int getYPositionOfThumbnail(int index) {
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


protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        int y = 0;
        int rowHeight = THUMB_HEIGHT;
        QString currentFolder;
        QList<CachedImage> rowPixmaps;
        int rowWidth = 0;

        m_thumbnailRects.clear(); // clear previous positions

        auto drawRow = [&](QList<CachedImage> &rowPixmaps, int y, int rowWidth) {
            int x = (width() - rowWidth) / 2;
            for (int i = 0; i < rowPixmaps.size(); ++i) {
                const CachedImage &rpix = rowPixmaps[i];
                int rw = rpix.pix.width() * rowHeight / rpix.pix.height();
                QRect thumbRect(x, y, rw, rowHeight);

                // Hover flash: subtle pulsing white overlay
                if (m_hovering && m_hoveredImage.filePath == rpix.filePath) {
                    int hoverAlpha = 40 + int(15 * std::sin(QDateTime::currentMSecsSinceEpoch() / 100.0));
                    QPixmap bright = rpix.pix;
                    QPainter tmp(&bright);
                    tmp.fillRect(bright.rect(), QColor(255, 255, 255, hoverAlpha));
                    tmp.end();
                    painter.drawPixmap(thumbRect, bright);
                } else {
                    painter.setOpacity(0.85);
                    painter.drawPixmap(thumbRect, rpix.pix);
                    painter.setOpacity(1.0);
                }

                // Click flash: temporary white overlay
                if (m_clickedIndex >= 0 &&
                    m_clickedIndex < m_pixmaps.size() &&
                    m_pixmaps[m_clickedIndex].filePath == rpix.filePath &&
                    m_clickFlashProgress > 0.0)
                {
                    painter.fillRect(thumbRect, QColor(255, 255, 255, int(100 * m_clickFlashProgress)));
                }

                m_thumbnailRects.append(thumbRect);
                x += rw + SPACING;
            }
        };

        for (const CachedImage &cimg : m_pixmaps) {
            int w = cimg.pix.width() * rowHeight / cimg.pix.height();

            // Folder gap
            if (!currentFolder.isEmpty() && currentFolder != cimg.folder) {
                drawRow(rowPixmaps, y, rowWidth);
                y += rowHeight + FOLDER_GAP;
                rowPixmaps.clear();
                rowWidth = 0;
            }
            currentFolder = cimg.folder;

            // Row wrap
            if (rowWidth + w + (rowPixmaps.size() > 0 ? SPACING : 0) > width()) {
                drawRow(rowPixmaps, y, rowWidth);
                y += rowHeight + SPACING;
                rowPixmaps.clear();
                rowWidth = 0;
            }

            rowPixmaps.append(cimg);
            rowWidth += w + (rowPixmaps.size() > 1 ? SPACING : 0);
        }

        // Last row
        if (!rowPixmaps.isEmpty()) {
            drawRow(rowPixmaps, y, rowWidth);
            y += rowHeight + SPACING;
        }

        setMinimumHeight(y);

        // Keep repainting if hover or click flash is active
        if (m_hovering || (m_clickedIndex >= 0 && m_clickFlashProgress > 0.0)) {
            update();
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        update();
    }

    void mousePressEvent(QMouseEvent *event) override {
        m_clickedIndex = -1;

        for (int i = 0; i < m_pixmaps.size(); ++i) {
            if (m_thumbnailRects[i].contains(event->pos())) {
                m_clickedIndex = i;
                m_clickFlashProgress = 1.0; // full flash
                QTimer::singleShot(150, this, [this]() {
                    m_clickFlashProgress = 0.0; // fade out
                    update();
                });
                update();
                break;
            }
        }
        if (m_clickedIndex >= 0 && m_clickedIndex < m_pixmaps.size()) {
            // QString dir = m_pixmaps[m_clickedIndex].folder; // Fine name only
            // QString dir = QFileInfo(m_pixmaps[m_clickedIndex].filePath).absolutePath();
            // QString filePath = m_pixmaps[m_clickedIndex].filePath;
            QString filePath = QFileInfo(m_pixmaps[m_clickedIndex].filePath).absoluteFilePath();
            QString monitor = m_currentMonitor; // now safe
            recordClick(monitor, filePath);
            updateHyprpaperWallpaper(monitor, filePath);
            // updateHyprpaperConf(); // Not Yet
        }
        QWidget::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        m_hovering = false;

        for (int i = 0; i < m_pixmaps.size(); ++i) {
            if (m_thumbnailRects[i].contains(event->pos())) {
                m_hovering = true;
                m_hoveredImage = m_pixmaps[i];
                m_hoveredRect = m_thumbnailRects[i];
                startHoverTimer();   // start pulsing
                break;
            }
        }

        if (!m_hovering) stopHoverTimer();  // stop pulsing if no hover
        update();
        QWidget::mouseMoveEvent(event);
    }

    


private:
    QList<CachedImage> m_pixmaps;
    QFileSystemWatcher *watcher;
    QString m_cacheFolder;
    QString m_mainFolder;
    QTimer hoverTimer;
    QString m_currentMonitor;

    // Hover tracking
    bool m_hovering = false;
    CachedImage m_hoveredImage;
    QRect m_hoveredRect;
    QList<QRect> m_thumbnailRects; // positions of all thumbnails

    int m_hoveringIndex = -1;
    int m_clickedIndex = -1;
    qreal m_clickFlashProgress = 0.0;

    void startHoverTimer() {
        if (!hoverTimer.isActive()) {
            connect(&hoverTimer, &QTimer::timeout, this, [=](){ update(); });
            hoverTimer.start(90); // ~30ms for smooth pulsing, but lets do 90
        }
    }

    void stopHoverTimer() {
        if (hoverTimer.isActive()) hoverTimer.stop();
    }

    void addWatchRecursive(const QString &path) {
        QDir dir(path);

        // Add current dir if not already watched
        if (!watcher->directories().contains(path))
            watcher->addPath(path);

        // Recurse into subdirs
        for (const QString &subdir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            addWatchRecursive(dir.absoluteFilePath(subdir));
        }
    }

    void onDirectoryChanged(const QString &path) {
        qDebug() << "Directory changed:" << path;

        // Reset watcher list
        watcher->removePaths(watcher->directories());
        addWatchRecursive(m_mainFolder);

        // Reload thumbnails
        loadFilteredPixmaps(m_cacheFolder, m_mainFolder);
        update();
    }

    void loadFilteredPixmaps(const QString &cacheFolder, const QString &mainFolder) {
        m_pixmaps.clear();
        QDirIterator it(mainFolder, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QFileInfo fi(filePath);
            QString folderName = fi.dir().dirName();

            QByteArray uri = ("file://" + fi.absoluteFilePath()).toUtf8();
            QString md5 = QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex();

            QString cachedPath = cacheFolder + "/" + md5 + ".png";
            if (QFile::exists(cachedPath)) {
                QPixmap pix(cachedPath);
                if (!pix.isNull()) m_pixmaps.append({pix, folderName, filePath});
            }
        }
    }
};


//////////////////////////////
////// APPLICATION GUI ///////
//////////////////////////////

int main(int argc, char *argv[]) {

    // Check for --cpu flag // Software CPU Render
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--cpu") {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
            break;
        }
    }
    
    QApplication app(argc, argv);
    QSettings settings("QtHyprpaper", "QtHyprpaperGUI");

    // Connect the aboutToQuit signal to updateHyprpaperConf
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        updateHyprpaperConf();  // write all clicks to hyprpaper.conf
    });

    app.setApplicationName("QtHyprpaperGUI"); 
    app.setApplicationDisplayName("Qt Hyprpaper GUI"); 

    loadLastClickedWallpapers();          // Step 1: load previous clicks
    QStringList monitors = getMonitorList(); // Step 2: preload monitors

    QHppQ *grid = new QHppQ(CACHE_FOLDER(), MAIN_FOLDER());

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidget(grid);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setAttribute(Qt::WA_TranslucentBackground);
    scroll->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    scroll->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 40px; }"
        "QScrollBar::handle:vertical { background: gray; border-radius: 10px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; }"
    );

    scroll->horizontalScrollBar()->setStyleSheet(
        "QScrollBar:horizontal { height: 40px; }"
        "QScrollBar::handle:horizontal { background: gray; border-radius: 10px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; }"
    );

    // Window Transcluency
    
    QWidget window;
    window.setAttribute(Qt::WA_TranslucentBackground);
    window.setWindowFlag(Qt::FramelessWindowHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(&window);
    // mainLayout->setContentsMargins(20, 20, 20, 20); // left, top, right, bottom
    mainLayout->setContentsMargins(WINDOW_PADDING, WINDOW_PADDING, WINDOW_PADDING, WINDOW_PADDING);
    mainLayout->addWidget(scroll);

    // Bottom controls layout
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(10, 10, 10, 10); // left, top, right, bottom

    // Zoom slider on left
    QSlider *zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setMinimum(64);
    zoomSlider->setMaximum(512);
    zoomSlider->setValue(THUMB_HEIGHT);
    zoomSlider->setFixedWidth(200); // Fixed Size

    // Restore zoom
    int savedZoom = settings.value("zoom", THUMB_HEIGHT).toInt();
    zoomSlider->setValue(savedZoom);
    THUMB_HEIGHT = savedZoom;

    QObject::connect(zoomSlider, &QSlider::valueChanged, [&](int value){
        // Find the thumbnail at viewport center
        int centerY = scroll->verticalScrollBar()->value() + scroll->viewport()->height()/2;
        int indexAtCenter = grid->getThumbnailIndexAtY(centerY);

        // Update zoom and repaint
        THUMB_HEIGHT = value;
        grid->update();

        // After layout, scroll to keep same thumbnail centered
        QTimer::singleShot(0, [=](){
            int newCenterY = grid->getYPositionOfThumbnail(indexAtCenter);
            scroll->verticalScrollBar()->setValue(newCenterY - scroll->viewport()->height()/2);
        });

        // Save zoom
        settings.setValue("zoom", value);
    });

    // Spacer in middle
    controlsLayout->addWidget(zoomSlider);
    controlsLayout->addStretch();

    // ComboBox on right
    QComboBox *combo = new QComboBox();

    // Populate combo with monitors detected by hyprctl
    combo->clear();
    for (const QString &m : monitors) {
        combo->addItem(m);
    }
    combo->setFixedWidth(100); // Fixed Size

    // Restore combobox selection if available
    int savedIndex = settings.value("comboIndex", -1).toInt();
    if (savedIndex >= 0 && savedIndex < combo->count()) {
        combo->setCurrentIndex(savedIndex);
        grid->setCurrentMonitor(combo->currentText());
    } else if (combo->count() > 0) {
        // fallback only if nothing saved
        combo->setCurrentIndex(0);
        grid->setCurrentMonitor(combo->currentText());
    }

    // Keep combobox and grid in sync
    QObject::connect(combo, &QComboBox::currentTextChanged,
                    grid, &QHppQ::setCurrentMonitor);

    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        settings.setValue("comboIndex", index);  // save selection
    });


    controlsLayout->addWidget(combo);
    controlsLayout->setContentsMargins(5, 5, 5, 5); // left, top, right, bottom

    mainLayout->addLayout(controlsLayout);


    window.setWindowTitle("Qt Hyprpaper GUI");
    window.resize(800, 600);
    window.show();

    return app.exec();
}

#include "main.moc"
