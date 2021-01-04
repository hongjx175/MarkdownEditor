#include "mdserver.h"
#include "ui_mdserver.h"
#include <QtNetwork>
#include <QFile>
#include <QList>
#include <QMessageBox>
#include <QSqlError>

MDserver::MDserver(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MDserver)
{
    tcpServer = new QTcpServer(this);
    tcpServer->listen(QHostAddress::Any,8888);

    tcpSocket = new QTcpSocket(this);
    ui->setupUi(this);
    setFixedSize(this->width(),this->height());

    ui->server_ip_line->setText(getHostIp());
    ui->server_port_line->setText("8888");
    ui->server_ip_line->setReadOnly(true);
    ui->client_ip_line->setReadOnly(true);
    ui->server_port_line->setReadOnly(true);
    ui->client_port_line->setReadOnly(true);
    connect(tcpServer,&QTcpServer::newConnection,this,&MDserver::connect_to_client);

    timer = new QTimer();

    bytesreceived = 0;
    bytesfilenamesize = 0;
    bytestotal = 0;


    hostName = "192.168.1.108";
    dbName = "markdown";
    userName = "root";
    password = "password";

    dbconn = QSqlDatabase::addDatabase("QMYSQL");
    dbconn.setHostName(hostName);
    dbconn.setDatabaseName(dbName);
    dbconn.setUserName(userName);
    dbconn.setPassword(password);

    qDebug("database open status: %d\n",dbconn.open());
    QSqlError sql_error = dbconn.lastError();
    qDebug()<<sql_error.text();
    dbconn.close();

}
QString MDserver::getHostIp()
{
    QString strIpAddress;
    QList<QHostAddress> ipAddressList = QNetworkInterface::allAddresses();
    int nListSize = ipAddressList.size();
    for(int i = 0;i < nListSize; i++)
    {
        if(ipAddressList.at(i) != QHostAddress::LocalHost && ipAddressList.at(i).toIPv4Address())
        {
            strIpAddress = ipAddressList.at(i).toString();
            break;
        }
    }
    if(strIpAddress.isEmpty())
        strIpAddress = QHostAddress::LocalHost;
    return strIpAddress;

}
MDserver::~MDserver()
{
    tcpServer->close();
    tcpSocket->close();
    delete tcpServer;
    delete tcpSocket;
    delete timer;
    delete ui;
}
void MDserver::m_disconnected()
{
     ui->textEdit->append(QDateTime::currentDateTime().toString()+"  ->  已断开Client");

}
void MDserver::connect_to_client()
{
    ui->textEdit->append(QDateTime::currentDateTime().toString()+"  ->  已连接Client");
    tcpSocket = tcpServer->nextPendingConnection();//设置监听
    ui->client_ip_line->setText(tcpSocket->peerAddress().toString().split("::ffff:").last());
    ui->client_port_line->setText(QString("%1").arg(tcpSocket->peerPort()));
    connect(tcpSocket,&QTcpSocket::readyRead,[this](){

        receiveFile();

    });
    connect(tcpSocket,&QTcpSocket::disconnected,this,&MDserver::m_disconnected);
}
void MDserver::sendFile()//发送文件
{
    QString pdfName = pdfFile.fileName().split("/").last();
    bool Ok = pdfFile.open(QIODevice::ReadOnly);
    ui->textEdit->append(pdfName);
    if(Ok == false)
    {
        ui->textEdit->append(QString("pdf文件 %1 打开失败").arg(pdfFile.fileName()));

    }else{
        QByteArray block;
        QByteArray data = pdfFile.readAll();
        QDataStream dts(&block,QIODevice::WriteOnly);
        dts<<qint64(0)<<qint64(0)<<QString(pdfName);
        dts.device()->seek(0);
        dts<<(qint64)(block.size() + pdfFile.size());
        dts<<(qint64)(block.size() - sizeof(qint64)*2);
        dts<<QString(pdfName);
        dts<<data;

        this->tcpSocket->write(block);
        ui->textEdit->append(QString("send %1 sucessfully").arg(pdfName));
    }
    pdfFile.close();
    pdfFile.remove();

}
void MDserver::receiveFile()
{
    QDataStream in(this->tcpSocket);
    if(bytesreceived == 0)
    {
        if(tcpSocket->bytesAvailable() >= 2*sizeof(qint64))
        {
            in>>bytestotal;
            in>>bytesfilenamesize;
            bytesreceived += 2*sizeof(qint64);

            if(tcpSocket->bytesAvailable() + bytesreceived >= bytestotal)
            {
                qDebug()<<"接收完成";
                in>>filename;
                ui->textEdit->append("receive " + filename + " sucessfully");
                QByteArray block;
                in>>block;

                file = new QFile(filename);
                qDebug()<<filename;
                if(!file->open(QIODevice::WriteOnly))
                {
                    qDebug()<<"write file error";
                    return;
                }else{
                    file->write(block);
                    file->close();
                }


                //接收md文件完成之后
                //md_into_sql();
                bytesreceived = 0;
                bytesfilenamesize = 0;
                bytestotal = 0;
                markdown_pdf();
                pdfFile.setFileName(file->fileName().split("/").last().remove(".md") + ".pdf");
                sendFile();
            }

        }
    }else{
        if(tcpSocket->bytesAvailable() + bytesreceived >= bytestotal)
        {
            qDebug()<<"接收完成";
            in>>filename;
            ui->textEdit->append("receive" + filename + "sucessfully");
            QByteArray block;
            in>>block;

            file = new QFile(filename);
            qDebug()<<filename;
            if(!file->open(QIODevice::WriteOnly))
            {
                qDebug()<<"write file error";
                return;
            }else{
                file->write(block);
                file->close();
            }


            //接收md文件完成之后
            //md_into_sql();//存入数据库sql
            bytesreceived = 0;
            bytesfilenamesize = 0;
            bytestotal = 0;
            markdown_pdf();
            pdfFile.setFileName(file->fileName().split("/").last().remove(".md") + ".pdf");
            sendFile();
        }
    }
}

void MDserver::md_into_sql()
{
    if(!dbconn.open())
    {
        ui->textEdit->append("数据库未打开");
        return;
    }

    QSqlQuery query;
    if(!file->open(QIODevice::ReadOnly))
    {
         ui->textEdit->append("markdown文件存入数据库失败");
         return;
    }

    QByteArray data = file->readAll();
    file->close();
    query.exec("create table if not exists markdowntable(mdname TEXT,mddata BLOB);");
    int exist = 0;
    query.exec(QString("select count(*) from markdowntable where mdname='%1';").arg(file->fileName().split("/").last()));
    while(query.next())
    if(query.isActive())
        exist = query.value(0).toInt();
    qDebug()<<exist;
    QString sql_option;
    bool sucess = true;
    if(exist == 0)
    {
        sql_option = "INSERT INTO markdowntable(mdname,mddata) VALUES(:mdname,:mddata);";
        query.prepare(sql_option);
        query.bindValue(":mdname",file->fileName().split("/").last());
        query.bindValue(":mddata",data);
        if(!query.exec())
        {
            qDebug()<<"插入失败";
            sucess = false;
            qDebug()<<query.lastError();
        }
    }else{
        sql_option = QString("DELETE FROM markdowntable WHERE mdname='%1'").arg(file->fileName().split("/").last());
        query.exec(sql_option);

        sql_option = QString("INSERT INTO markdowntable (mdname,mddata) VALUES(:mdname,:mddata);");
        query.prepare(sql_option);
        query.bindValue(":mdname",file->fileName().split("/").last());
        query.bindValue(":mddata",data);
        if(!query.exec())
        {
            qDebug()<<"更新失败";
            //qDebug()<<sql_option;
            sucess = false;
            qDebug()<<query.lastError();
        }
    }
    if(sucess) ui->textEdit->append("insert into or update sucessful!");
    else  ui->textEdit->append("insert into or update fail!");
}

void MDserver::markdown_pdf()
{
   system(("markdown-pdf " + file->fileName()).toLatin1().data());
}
