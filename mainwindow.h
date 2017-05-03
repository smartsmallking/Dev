#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "QDomDocument"
#include "QTcpServer"
#include <QUdpSocket>
#include <QHostInfo>
#include "QHostAddress"
#include "QTimer"
#include "QDateTime"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int i=1;
    QByteArray parseByte;
    QByteArray RequestXml,ResponseXml;

    void parseNode(QDomNode node);
    QDomDocument *doc;
    QDomNodeList list;

    QUdpSocket *udpSocket;
    QTcpSocket *tcpSocket;
    quint16 port=6000;
    QByteArray data[40];
    QHostAddress address;
    QString currentDeviceIp;
    QTimer *timer;
    bool isFinished=false;
    int a,b,c,d;
    int a1,b1,c1,d1;
    int count=0;

    bool state=false,entry=false,output=false,input=false;
    double outputTime,inputTime;
    double outputTime2,inputTime2;

private:
    Ui::MainWindow *ui;

    QTcpSocket  *m_pSocket,*m_tcpsocket;



private slots:
    void readMessage();
    void on_parseXML_clicked();
    void sendCommand();
    void on_parseAll_btn_clicked();
    void on_configure_clicked();
    void on_statusSetting_clicked();
    void processPendingDatagrams();
    void on_changeIp_btn_clicked();
    void on_forceConnect_btn_clicked();
    void on_configure1_clicked();
    void on_statusSetting1_clicked();
};

#endif // MAINWINDOW_H
