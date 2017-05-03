#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QByteArray>
#include <QHostAddress>
#include "QDebug"
#include "QTimer"
#include "QFile"
#include "QDateTime"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnSend,SIGNAL(clicked()),this,SLOT(sendCommand()));

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(port, QUdpSocket::ShareAddress);
    udpSocket->open(QUdpSocket::ReadWrite);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));


    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),ui->textEdit_udp,SLOT(clear()));
    connect(ui->clear,SIGNAL(clicked()),ui->textEdit,SLOT(clear()));


    m_pSocket = new QTcpSocket(this);
    connect(m_pSocket,SIGNAL(readyRead()),this,SLOT(readMessage()));
}

MainWindow::~MainWindow()
{
    m_pSocket->disconnectFromHost();
    m_pSocket->deleteLater();
    delete ui;
}

//发送指令
void MainWindow::sendCommand()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();
    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(512);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;//type通用消息

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    QString com=ui->comboBox->currentText();
    char *theBuf;
    QByteArray buf="<Message namespace=\"ZH-606\" version=\"1.2\" type=\"request\" sequence=\"1\">"
                   "<Execute command=\""+com.toLatin1()+"\" enforce=\"true\"/></Message>";
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n"+buf));
    RequestXml=buf;
}

//接收tcp消息
void MainWindow::readMessage()
{
    while(m_pSocket->bytesAvailable()>0)
    {
        QByteArray data=m_pSocket->readAll();

        qDebug()<<data;
        if(data.contains("TXML"))
        {
            int headPos=data.indexOf("<Message",0);
            int tailPos=data.indexOf("</Message>",0);
            int lastPos=data.lastIndexOf("</Message>");
            if(tailPos!=lastPos)
            {
                QByteArray allData=data.mid(headPos,lastPos-headPos+10);
                int p=-10,q=-10;
                int l=allData.lastIndexOf("</Message>");
                for(int i=0;i<10;i++)
                {
                    p=allData.indexOf("<Message",p+10);
                    q=allData.indexOf("</Message>",q+10);
                    QByteArray temp=allData.mid(p,q-p+10);
                    //解析temp
                    doc=new QDomDocument("mydocument");
                    if (!doc->setContent(temp))
                    {
                        return;
                    }
                    QDomElement root = doc->documentElement();
                    list=root.parentNode().childNodes();
                    qDebug()<<"listcount:"<<list.count();
                    for(int i=0;i<list.count();i++)
                    {
                        state=false;
                        entry=false;
                        output=false;
                        input=false;
                        QDomNode node=list.at(i);
                        qDebug()<<"-----------------";
                        parseNode(node);

                    }
                    //到达最后，停止解析
                    if(q==l)
                    {
                        return;
                    }
                }
            }
            ui->textEdit->append(tr("respond:\n"+data.mid(headPos,tailPos-headPos+10)));
            parseByte=data.mid(headPos,tailPos-headPos+10);
            ResponseXml=parseByte;
        }
    }
}

//解析发送报文
void MainWindow::on_parseXML_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignLeft);
    doc=new QDomDocument("mydocument");
    if (!doc->setContent(RequestXml))
    {
        return;
    }
    QDomElement root = doc->documentElement();
    list=root.parentNode().childNodes();
    qDebug()<<"list count:"<<list.count();
    for(int i=0;i<list.count();i++)
    {
        QDomNode node=list.at(i);
        parseNode(node);
        ui->textEdit->append("\n");
    }
}

//解析接收的报文
void MainWindow::on_parseAll_btn_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignRight);
    doc=new QDomDocument("mydocument");
    if (!doc->setContent(ResponseXml))
    {
        return;
    }
    QDomElement root = doc->documentElement();
    list=root.parentNode().childNodes();
    for(int i=0;i<list.count();i++)
    {
        QDomNode node=list.at(i);
        parseNode(node);
        ui->textEdit->append("\n");
    }
}

void MainWindow::parseNode(QDomNode node)
{
    QDomNamedNodeMap map=node.attributes();
    ui->textEdit->append("----------"+node.nodeName()+"----------");
    if(node.nodeName()=="State")
    {
        qDebug()<<"1";
        state=true;
    }
    if(node.nodeName()=="Entry")
    {
        qDebug()<<"2";
        entry=true;
    }
    if(node.nodeName()=="Output")
    {
        qDebug()<<"3";
        output=true;
    }
    if(node.nodeName()=="Input")
    {
        qDebug()<<"4";
        input=true;
    }
    for(int i=0;i<map.count();i++)
    {
        ui->textEdit->append(map.item(i).nodeName()+":"+map.item(i).nodeValue());
        if(map.item(i).nodeName()=="nanosecond")
        {
            if(state&&entry)
            {
                qDebug()<<"start:"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ")+map.item(i).nodeValue();
            }
            if(output)
            {
                qDebug()<<"开出量动作时间:"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ")+map.item(i).nodeValue();
                outputTime=map.item(i).nodeValue().toDouble();
            }
            if(input)
            {
                qDebug()<<"开入量动作时间:"<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ")+map.item(i).nodeValue();
                inputTime=map.item(i).nodeValue().toDouble();
            }
        }
        if(map.item(i).nodeName()=="second")
        {
            if(output)
            {
                outputTime2=map.item(i).nodeValue().toDouble();
            }
            if(input)
            {
                inputTime2=map.item(i).nodeValue().toDouble();
            }
        }
        if(outputTime!=0&&inputTime!=0&&outputTime2!=0&&inputTime2!=0)
        {
            qDebug()<<"动作时间:"<<QString::number(((inputTime2-outputTime2)*1000000+inputTime-outputTime)/1000000, 'f', 3);
            outputTime=0;
            inputTime=0;
            outputTime2=0;
            inputTime2=0;
        }
    }
    if(node.hasChildNodes())
    {
        for(int i=0;i<node.childNodes().count();i++)
        {
            parseNode(node.childNodes().at(i));
        }
    }
}

//configure
void MainWindow::on_configure_clicked()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();

    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(100024);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    char *theBuf;
    QByteArray buf;
    QFile *file=new QFile("D:/configure.xml");
    if(file->open(QIODevice::ReadWrite|QIODevice::Text))
    {
        buf=file->readAll().data();
    }
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n")+theBuf);
    RequestXml=theBuf;
}

//statusSettig
void MainWindow::on_statusSetting_clicked()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();

    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(100024);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    char *theBuf;
    QByteArray buf;
    QFile *file=new QFile("D:/statessetting.xml");
    if(file->open(QIODevice::ReadWrite|QIODevice::Text))
    {
        buf=file->readAll().data();
    }
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n")+theBuf);
    RequestXml=theBuf;
}
//接收udp报文
void MainWindow::processPendingDatagrams()
{
    timer->stop();
    timer->start(6000);
    while(udpSocket->hasPendingDatagrams())
    {
        ui->textEdit_udp->clear();
        QByteArray datagram;
        QHostAddress ip;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),udpSocket->pendingDatagramSize(),&ip,&port);
        int length=datagram.size();
        ui->textEdit_udp->append("接收到报文："+datagram.toHex());
        ui->textEdit_udp->append("报文长度："+QString::number(length));
        ui->textEdit_udp->append("来源ip："+ip.toString().mid(7,16));
        address.setAddress(ip.toString().mid(7,16));
        currentDeviceIp=ip.toString().mid(7,16);
        ui->textEdit_udp->append("_____________________________");
        ui->textEdit_udp->append("广播类别："+datagram.toHex().left(8));
        ui->textEdit_udp->append("版本："+datagram.toHex().mid(8,8));
        ui->textEdit_udp->append("附带信息:"+datagram.toHex().mid(16,8));
        ui->textEdit_udp->append("唯一标识："+datagram.toHex().mid(24,8).toUpper());
        ui->textEdit_udp->append("保留：:"+datagram.toHex().mid(32,8));
        ui->textEdit_udp->append("HappenTime:"+datagram.toHex().right(4));
    }
}

//更改下位机ip地址
void MainWindow::on_changeIp_btn_clicked()
{
    QString ip=ui->lineEdit_changeIp->text();
    QString ip1=ip.left(ip.indexOf("."));
    ip=ip.right(ip.length()-ip.indexOf(".")-1);
    QString ip2=ip.left(ip.indexOf("."));
    ip=ip.right(ip.length()-ip.indexOf(".")-1);
    QString ip3=ip.left(ip.indexOf("."));
    ip=ip.right(ip.length()-ip.indexOf(".")-1);
    QString ip4=ip.left(ip.indexOf("."));
    ip=ip.right(ip.length()-ip.indexOf(".")-1);

    ip=QString("%1").arg(ip1.toInt(),2,16,QLatin1Char('0'))
            +QString("%1").arg(ip2.toInt(),2,16,QLatin1Char('0'))
            +QString("%1").arg(ip3.toInt(),2,16,QLatin1Char('0'))
            +QString("%1").arg(ip4.toInt(),2,16,QLatin1Char('0'));;
    qDebug()<<"ip:"<<ip;
    QByteArray cmd="00000001 00000000 00000000 c0a801c6 "+ip.toLatin1()+" 00000000";
    ui->textEdit->append(tr("changeIp:"+cmd));
    QUdpSocket *MySocket=new QUdpSocket;
    QByteArray MyData=QByteArray::fromHex(cmd);
    MySocket->writeDatagram(MyData,address,6001);
}

//强制连接
void MainWindow::on_forceConnect_btn_clicked()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();

    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(100024);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    char *theBuf;
    QByteArray buf;
    QFile *file=new QFile("D:/forceConnect.xml");
    if(file->open(QIODevice::ReadWrite|QIODevice::Text))
    {
        buf=file->readAll().data();
    }
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n")+theBuf);
    RequestXml=theBuf;
}

void MainWindow::on_configure1_clicked()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();

    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(100024);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    char *theBuf;
    QByteArray buf;
    QFile *file=new QFile("D:/stateChange.xml");
    if(file->open(QIODevice::ReadWrite|QIODevice::Text))
    {
        buf=file->readAll().data();
    }
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n")+theBuf);
    RequestXml=theBuf;
}

void MainWindow::on_statusSetting1_clicked()
{
    m_pSocket->connectToHost(address, 6000);
    m_pSocket->waitForConnected();

    ui->textEdit->setAlignment(Qt::AlignLeft);
    int i = 0;

    int iSize = 0;
    int iBufLen = 0;

    QByteArray arrBuf;
    arrBuf.resize(100024);
    arrBuf[i++] = 0xFA;
    arrBuf[i++] = 0xCE;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01; // target对应连接上位机在本地的SOCKET句柄

    iSize = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x1C;//size从sender到SliceNum段的数据大小


    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12; // sender本地的SOCKET句柄

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x00;//seqnum 报文序号，自增长，从0开始

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//topiclen消息类型长度，默认为4

    iBufLen = i;
    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x04;//buflen消息数据长度

    sprintf(arrBuf.data()+i, "TXML");

    i += 4;
    char *theBuf;
    QByteArray buf;
    QFile *file=new QFile("D:/statussetting1.xml");
    if(file->open(QIODevice::ReadWrite|QIODevice::Text))
    {
        buf=file->readAll().data();
    }
    theBuf=buf.data();
    quint16 iTmp = strlen(theBuf);
    sprintf(arrBuf.data()+i, theBuf);
    i += iTmp;
    arrBuf[iBufLen] = (quint8)(iTmp>>8);
    arrBuf[iBufLen+1] = (quint8)(iTmp);

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x12;

    arrBuf[i++] = 0x00;
    arrBuf[i++] = 0x01;

    arrBuf[iSize] = (quint8)((i-8)>>8);
    arrBuf[iSize+1] = (quint8)(i-8);

    m_pSocket->write(arrBuf.data(), i);
    ui->textEdit->append(tr("resquest:\n")+theBuf);
    RequestXml=theBuf;
}
