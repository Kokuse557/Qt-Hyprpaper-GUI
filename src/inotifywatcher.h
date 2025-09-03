// inotifywatcher.h
#pragma once
#include <QObject>
#include <QHash>
#include <QSocketNotifier>
#include <QString>

class InotifyWatcher : public QObject {
    Q_OBJECT
public:
    explicit InotifyWatcher(const QString &path, QObject *parent = nullptr);
    ~InotifyWatcher();

    void addWatch(const QString &path);

signals:
    void fileCreated(const QString &path);
    void fileDeleted(const QString &path);

private slots:
    void processEvents();

private:
    int fd;
    QSocketNotifier *notifier;
    QHash<int, QString> watchDescriptors;
};
