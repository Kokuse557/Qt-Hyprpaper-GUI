#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QSet>
#include <QDebug>
#include <QDir>
#include <QString>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "paths.h"

// Map: monitor → last clicked wallpaper file
static QMap<QString, QString> lastClickedWallpapers;

void recordClick(const QString &monitor, const QString &filePath) {
    if (!monitor.isEmpty() && !filePath.isEmpty())
        lastClickedWallpapers[monitor] = filePath;
}

// -------------------------------
// Monitor fetcher
// -------------------------------
// get monitors from hyprctl
QStringList getMonitorList() {
    QStringList monitors;
    QProcess proc;
    proc.start("hyprctl", QStringList() << "monitors" << "-j");
    proc.waitForFinished();

    QString output = proc.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());

    if (doc.isArray()) {
        for (const QJsonValue &val : doc.array()) {
            if (val.isObject()) {
                QJsonObject mon = val.toObject();
                if (mon.contains("name"))
                    monitors.append(mon["name"].toString());
            }
        }
    }
    return monitors;
}

// initialize lastClickedWallpapers from hyprpaper.conf

void loadLastClickedWallpapers() {
    lastClickedWallpapers.clear();
    QFile f(HYPRPAPER_CONF());
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open" << HYPRPAPER_CONF();
        return;
    }
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.startsWith("wallpaper", Qt::CaseInsensitive)) continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString rhs = line.mid(eq + 1).trimmed();
        int comma = rhs.indexOf(',');
        if (comma < 0) continue;
        QString mon = rhs.left(comma).trimmed();
        QString path = rhs.mid(comma + 1).trimmed();
        if (!mon.isEmpty() && !path.isEmpty())
            lastClickedWallpapers[mon] = path;
    }
    f.close();

    // 🔹 Extra: RAM CLEANUP TIME 
    // int ret = QProcess::execute("hyprctl", {"hyprpaper", "unload", "unused"});
    // if (ret != 0) {
    //   qWarning() << "Failed to run hyprctl hyprpaper unload unused";
    //} else {
    //    qDebug() << "Hyprpaper: unloaded unused wallpapers";
    //}
}

// -------------------------
// Update hyprpaper realtime
// -------------------------

// Robustly run hyprctl preload + wallpaper. Try both comma formats if needed.
void updateHyprpaperWallpaper(const QString &monitor, const QString &filePath) {
    if (monitor.isEmpty() || filePath.isEmpty()) return;

    // preload
    {
        QProcess p;
        p.start("hyprctl", QStringList() << "hyprpaper" << "preload" << filePath);
        if (!p.waitForFinished(1000)) p.waitForFinished();
        QString out = p.readAllStandardOutput() + p.readAllStandardError();
        qDebug() << "Preload output:" << out.trimmed();
    }

    // wallpaper: attempt without space, then with space if hyprctl reports unknown request
    QString argNoSpace = QString("%1,%2").arg(monitor, filePath);
    QString argWithSpace = QString("%1, %2").arg(monitor, filePath);

    QProcess p2;
    p2.start("hyprctl", QStringList() << "hyprpaper" << "wallpaper" << argNoSpace);
    if (!p2.waitForFinished(1000)) p2.waitForFinished();
    QString out2 = p2.readAllStandardOutput() + p2.readAllStandardError();
    qDebug() << "Wallpaper set (no-space) output:" << out2.trimmed();

    if (out2.contains("unknown request", Qt::CaseInsensitive)) {
        QProcess p3;
        p3.start("hyprctl", QStringList() << "hyprpaper" << "wallpaper" << argWithSpace);
        if (!p3.waitForFinished(1000)) p3.waitForFinished();
        QString out3 = p3.readAllStandardOutput() + p3.readAllStandardError();
        qDebug() << "Wallpaper set (with-space) output:" << out3.trimmed();
    }

    // update in-memory record
    lastClickedWallpapers[monitor] = filePath;
}

// -------------------------------
// Update hyprpaper.conf
// -------------------------------

// Write hyprpaper.conf placing preload lines at lines 11..20 and wallpaper lines at 21..30
void updateHyprpaperConf() {
    QFile file(HYPRPAPER_CONF());
    QStringList originalLines;
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open" << HYPRPAPER_CONF();
            return;
        }
        QTextStream in(&file);
        while (!in.atEnd()) originalLines.append(in.readLine());
        file.close();
    }

    // Keep everything except preload/wallpaper
    QStringList base;
    for (const QString &ln : originalLines) {
        QString t = ln.trimmed();
        if (!t.startsWith("preload", Qt::CaseInsensitive) &&
            !t.startsWith("wallpaper", Qt::CaseInsensitive)) {
            base.append(ln);
        }
    }

    // constants
    const int HEADER_LINE = 7;  // Start From
    QStringList headerBlock = {
        "///-------------------------------->",
        "/// REGENERATED BY QT_HYPRPAPER_GUI",
        "///-------------------------------->"
    };

    // Insert header if not already present
    bool headerExists = false;
    for (int i = 0; i < base.size() && i < HEADER_LINE + headerBlock.size(); ++i) {
        if (base[i].contains("REGENERATED BY QT_HYPRPAPER_GUI")) {
            headerExists = true;
            break;
        }
    }
    if (!headerExists) {
        while (base.size() < HEADER_LINE) base.append(QString());
        for (int i = 0; i < headerBlock.size(); ++i) {
            if (HEADER_LINE + i < base.size())
                base[HEADER_LINE + i] = headerBlock[i];
            else
                base.append(headerBlock[i]);
        }
    }

    // constants: reserved slots
    const int PRELOAD_START   = 10; // line 11 (1-based)
    const int PRELOAD_COUNT   = 10;
    const int WALLPAPER_START = 20; // line 21 (1-based)
    const int WALLPAPER_COUNT = 10;

    // Ensure enough lines
    while (base.size() < WALLPAPER_START + WALLPAPER_COUNT)
        base.append(QString());

    QStringList monitors = getMonitorList();

    // Preload slots
    for (int i = 0; i < PRELOAD_COUNT; ++i) {
        QString l;
        if (i < monitors.size()) {
            QString mon  = monitors[i];
            QString full = lastClickedWallpapers.value(mon);
            if (!full.isEmpty())
                l = QString("preload = %1").arg(full); // file, not dir
            else
                l = QString();
        }
        base[PRELOAD_START + i] = l;
    }

    // Wallpaper slots
    for (int i = 0; i < WALLPAPER_COUNT; ++i) {
        QString l;
        if (i < monitors.size()) {
            QString mon  = monitors[i];
            QString full = lastClickedWallpapers.value(mon);
            if (!full.isEmpty())
                l = QString("wallpaper = %1,%2").arg(mon, full);
            else
                l = QString("wallpaper = %1,").arg(mon);
        }
        base[WALLPAPER_START + i] = l;
    }

    // Write back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Failed to write" << HYPRPAPER_CONF();
        return;
    }
    QTextStream out(&file);
    for (const QString &ln : base) out << ln << "\n";
    file.close();

    // 🔹 Extra: RAM CLEANUP TIME 
    int ret = QProcess::execute("hyprctl", {"hyprpaper", "unload", "unused"});
    if (ret != 0) {
        qWarning() << "Failed to run hyprctl hyprpaper unload unused";
    } else {
        qDebug() << "RAM cleaning :  unloaded unused wallpapers";
    }

    qDebug() << "hyprpaper.conf updated";
}



