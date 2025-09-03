// inotifywatcher.cpp
#include "inotifywatcher.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <QDebug>

InotifyWatcher::InotifyWatcher(const QString &path, QObject *parent)
    : QObject(parent)
{
    fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        perror("inotify_init1");
        return;
    }

    addWatch(path);

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &InotifyWatcher::processEvents);
}

InotifyWatcher::~InotifyWatcher() {
    if (notifier) delete notifier;
    if (fd >= 0) close(fd);
}

void InotifyWatcher::addWatch(const QString &path) {
    int wd = inotify_add_watch(fd, path.toUtf8().constData(),
                               IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0) {
        perror("inotify_add_watch");
    } else {
        watchDescriptors[wd] = path;
    }
}

void InotifyWatcher::processEvents() {
    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;

    while ((len = read(fd, buf, sizeof buf)) > 0) {
        for (char *ptr = buf; ptr < buf + len; ) {
            struct inotify_event *event = (struct inotify_event *) ptr;
            QString base = watchDescriptors[event->wd];
            QString full = base + "/" + QString::fromUtf8(event->name);

            if (event->mask & (IN_CREATE | IN_MOVED_TO)) emit fileCreated(full);
            if (event->mask & (IN_DELETE | IN_MOVED_FROM)) emit fileDeleted(full);

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}
