#ifndef MDEDITOR_H
#define MDEDITOR_H

#include <QMainWindow>
#include <QFile>
#include "document.h"
#include <QLabel>
#include <QtNetwork>
#include <mysql.h>

class QTcpSocket;
QT_BEGIN_NAMESPACE
namespace Ui { class MDeditor; }
QT_END_NAMESPACE

class MDeditor : public QMainWindow
{
    Q_OBJECT

public:
    MDeditor(QWidget *parent = nullptr);
    ~MDeditor();

private:
    Ui::MDeditor *ui;
    QString MDfileName;
    QString MDfilePath;

    QFile MDfile;
    QFile tmpFile;
    bool changed;
    QTcpSocket *tcpSocket;
    QTcpSocket *fileSocket;
    Document m_content;
    QLabel* status_label;   
    QByteArray outBlock;    
    QString pic_path;

    QFile pdfFile;
    qint64 receiveSize;

    qint64 bytesreceived;
    qint64 bytesfilenamesize;
    qint64 bytestotal;
    QString filename;
    QFile* file;  

    MySQL *mysql;

    bool createDir();
    void new_file();
    void get_pic_path();
    void sendData();
    void sql_init();



private slots:
    void receiveData();
    void socket_connected();
    void closeEvent(QCloseEvent *event);
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveas_triggered();
    void on_actionExit_triggered();
    void on_actionB_triggered();
    void on_actionI_triggered();
    void on_actionDownline_triggered();
    void on_actionDelete_triggered();
    void on_actionLink_triggered();
    void on_actionCode_triggered();
    void on_actionPic_triggered();
    void on_actionH1_triggered();
    void on_actionH2_triggered();
    void on_actionRefer_triggered();
    void on_actionContact_triggered();
    void on_actionList_triggered();
    void on_actionNumList_triggered();
    void on_action_pdf_triggered();
    void on_textEdit_textChanged();
    void on_actionLine_triggered();
    void deal(QNetworkReply* reply);
    void on_action_cnnct_triggered();
    void on_actionMySQL_triggered();
    void receive(QString str);
    void on_actionTex_triggered();
};
#endif // MDEDITOR_H
