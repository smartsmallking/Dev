#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QThread>

class thread_udp : public QThread
{
    Q_OBJECT

public:
    thread_udp();
    void run();

};

#endif // MAINWINDOW_H
