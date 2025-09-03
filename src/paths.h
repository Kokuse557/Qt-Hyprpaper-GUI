#pragma once
#include <QDir>
#include <QString>

inline QString MAIN_FOLDER() { 
    return QDir::homePath() + "/Pictures/Wallpapers"; 
}
inline QString CACHE_FOLDER() { 
    return QDir::homePath() + "/.cache/thumbnails/large"; 
}
inline QString HYPRPAPER_CONF() { 
    return QDir::homePath() + "/.config/hypr/hyprpaper.conf"; 
}
