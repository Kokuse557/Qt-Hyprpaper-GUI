#pragma once
#include <QString>
#include <QPixmap>

struct CachedImage {
    QPixmap pix;
    QString folder;
    QString filePath;

    bool operator==(const CachedImage &other) const {
        return filePath == other.filePath;
    }
};
