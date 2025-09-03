#pragma once

#include <QString>
#include <QStringList>


// Click tracking
void recordClick(const QString &monitor, const QString &filePath);

// Config updates
void updateHyprpaperConf();

// Preload helpers
void loadLastClickedWallpapers();

// Wallpaper Set
void updateHyprpaperWallpaper(const QString &monitor, const QString &filePath);

// Monitor List
QStringList getMonitorList();
