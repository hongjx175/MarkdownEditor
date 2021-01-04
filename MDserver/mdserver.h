#ifndef MDSERVER_H
#define MDSERVER_H

#include <QMainWindow>
//#include <QtNetwork>
#include <QFile>
#include <QTimer>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
class QTcpServer;
class QTcpSocket;
QT_BEGIN_NAMESPACE
namespace Ui { class MDserver; }
QT_END_NAMESPACE

class MDserver : public QMainWindow
{
    Q_OBJECT

public:
    MDserver(QWidget *parent = nullptr);
    ~MDserver();
    void sendFile();
    void receiveFile();
    void markdown_pdf();

private:
    Ui::MDserver *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QString getHostIp();
    QFile pdfFile;
    QTimer *timer;

    qint64 bytesreceived;
    qint64 bytesfilenamesize;
    qint64 bytestotal;
    QString filename;
    QFile* file;

    QString hostName;
    QString dbName;
    QString userName;
    QString password;
    QSqlDatabase dbconn;

    void md_into_sql();

private slots:
    void connect_to_client();
    void m_disconnected();

};
#endif // MDSERVER_H
