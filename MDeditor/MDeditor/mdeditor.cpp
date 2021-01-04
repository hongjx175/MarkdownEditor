#include "mdeditor.h"
#include "ui_mdeditor.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QtNetwork>
#include "document.h"
#include "previewpage.h"
#include <QWebChannel>
#include <QLabel>
#include <QDesktopServices>
#include <QSqlError>
#include <mysql.h>
#include <QWebEngineSettings>

MDeditor::MDeditor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MDeditor)
{
    ui->setupUi(this);
    //connect(ui->actionOpen,SIGNAL(trggied()),this,SLOT(open()));
    MDfileName = nullptr;
    MDfilePath = nullptr;
    QString curPath = QCoreApplication::applicationDirPath();
    curPath = curPath.mid(0,curPath.lastIndexOf("/")+1);
    QDateTime curDateTime = QDateTime::currentDateTime();

    tmpFile.setFileName(curPath + "tmp_" + curDateTime.toString() +".md");
    ui->textEdit->setReadOnly(true);
    changed = false;

    tcpSocket = new QTcpSocket(this);
    //tcpSocket->abort();
    QString server_ip = "192.168.1.108";
    qint16 server_port = 8888;


    tcpSocket->connectToHost((QHostAddress)server_ip,server_port);
    connect(tcpSocket,&QTcpSocket::connected,this,&MDeditor::socket_connected);


    //预览
    ui->preview->setContextMenuPolicy(Qt::NoContextMenu);

    PreviewPage *page = new PreviewPage(this);
    ui->preview->setPage(page);

    //设置webeigneiew字体大小
    ui->preview->setZoomFactor(1.2);
    //QWebEngineSettings *defaultsetting = QWebEngineSettings::globalSettings();
    //defaultsetting->setFontSize(QWebEngineSettings::MinimumFontSize,35);

    //defaultsetting->resetFontFamily(ui->preview->font());

    connect(ui->textEdit, &QTextEdit::textChanged,
            [this]() { m_content.setText(ui->textEdit->toPlainText()); });
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(channel);

    ui->preview->setUrl(QUrl("qrc:/index.html"));


    status_label = NULL;


    createDir();
    if(status_label != NULL) delete status_label;
    status_label = new QLabel("未连接Server",this);
    ui->statusbar->addPermanentWidget(status_label);


    bytesreceived = 0;
    bytesfilenamesize = 0;
    bytestotal = 0;

    QFont font = ui->textEdit->font();
    font.setWeight(font.weight() * 1);
    font.setPointSize(font.pointSize() * 1.3);
    ui->textEdit->setFont(font);
}



void MDeditor::socket_connected()
{

    //tcpSocket->write("1");
    if(status_label != NULL) delete status_label;
    status_label = new QLabel("已连接Server",this);
    ui->statusbar->addPermanentWidget(status_label);

    connect(tcpSocket,&QTcpSocket::disconnected,[this](){
        if(status_label != NULL) delete status_label;
        status_label = new QLabel("已断开Server",this);
        ui->statusbar->addPermanentWidget(status_label);
    });
    connect(tcpSocket,&QTcpSocket::readyRead,this,&MDeditor::receiveData);
}

void MDeditor::sendData()
{

    bool Ok = MDfile.open(QIODevice::ReadOnly);
    qDebug()<<(MDfileName);
    if(Ok == false)
    {
        QMessageBox::information(this,"markdown打开失败","markdown打开失败");
        return;

    }else{
        QByteArray block;
        QByteArray data = MDfile.readAll();
        QDataStream dts(&block,QIODevice::WriteOnly);
        dts<<qint64(0)<<qint64(0)<<QString(MDfileName);
        dts.device()->seek(0);
        dts<<(qint64)(block.size() + MDfile.size());
        dts<<(qint64)(block.size() - sizeof(qint64)*2);
        dts<<QString(MDfileName);
        dts<<data;

        this->tcpSocket->write(block);
        qDebug()<<(QString("send %1 sucessfully").arg(MDfileName));
    }
    MDfile.close();
}
void MDeditor::receiveData()
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
                qDebug()<<filename;
                QByteArray block;
                in>>block;

                file = new QFile("pdfs/"+ filename);
                if(!file->open(QIODevice::WriteOnly))
                {
                    qDebug()<<"write file error";
                    return;
                }else{
                    file->write(block);
                    file->close();
                }

                //为下次接收初始化
                bytesreceived = 0;
                bytesfilenamesize = 0;
                bytestotal = 0;

            }
        }
    }else{
        if(tcpSocket->bytesAvailable() + bytesreceived >= bytestotal)
        {
            qDebug()<<"接收完成";
            in>>filename;
            qDebug()<<filename;
            QByteArray block;
            in>>block;

            file = new QFile("pdfs/"+ filename);
            qDebug()<<file->fileName();
            if(!file->open(QIODevice::WriteOnly))
            {
                qDebug()<<"write file error";
                return;
            }else{
                file->write(block);
                file->close();
            }

            //为下次接收初始化
            bytesreceived = 0;
            bytesfilenamesize = 0;
            bytestotal = 0;
        }
    }

}

bool MDeditor::createDir()
{
    QDir dir;
    return dir.mkpath("pdfs");
}

MDeditor::~MDeditor()
{

    delete tcpSocket;
    delete ui;
    delete status_label;
    delete mysql;
}

void MDeditor::closeEvent(QCloseEvent *event)
{
    if(changed == true)
    {
        on_actionExit_triggered();
    }
    tcpSocket->disconnectFromHost();
    tcpSocket->close();
    QFile::remove(tmpFile.fileName());
}

void MDeditor::on_actionNew_triggered()
{
    ui->textEdit->setReadOnly(false);

   // 如果正在输入一个文件，询问是否保存
    if(changed == true)
    {
        int ok = QMessageBox::information(this,"Save or not","是否保存当前文件？",QMessageBox::Yes|QMessageBox::No);
        if(ok == QMessageBox::Yes){
            if(MDfilePath == nullptr){
                MDeditor::new_file();
            }
            if(!MDfile.open(QFile::WriteOnly))
                return;
            QTextStream stream(&MDfile);
            stream<<ui->textEdit->toPlainText();
            MDfile.close();
        }
    }


    MDfileName = nullptr;
    MDfilePath = nullptr;
    ui->textEdit->clear();
}

void MDeditor::new_file()//创建一个新文件
{
    QString curPath = QCoreApplication::applicationDirPath();
    MDfilePath = QFileDialog::getSaveFileName(this,"新建文件","/","*.md");

    MDfileName = MDfilePath.split("/").last();

    MDfile.setFileName(MDfilePath);
}

void MDeditor::on_actionOpen_triggered()
{
    on_actionExit_triggered();
    ui->textEdit->setReadOnly(false);
    MDfilePath = QFileDialog::getOpenFileName(this,"打开文件","/","*.md");
    MDfileName = MDfilePath.split("/").last();
    MDfile.setFileName(MDfilePath);
    if(MDfile.open(QFile::ReadOnly)){
        QTextStream stream(&MDfile);
        ui->textEdit->setText(stream.readAll());
    }
    MDfile.close();
    changed = false;
}

void MDeditor::on_actionSave_triggered()
{
    if(MDfilePath == nullptr)
    {
        MDeditor::new_file();
    }

    if(!MDfile.open(QFile::ReadWrite))
        return;

    QTextStream stream(&MDfile);
    stream<<ui->textEdit->toPlainText();

    MDfile.close();
    changed = false;
    sendData();
}


void MDeditor::on_actionSaveas_triggered()
{
    if(MDfilePath == nullptr)
    {
        on_actionSave_triggered();
        return;
    }
    else{
        on_actionSave_triggered();
        MDfilePath = nullptr;
        on_actionSave_triggered();
    }

}

void MDeditor::on_actionExit_triggered()
{
    if(changed == true){
        int ok = QMessageBox::information(this,"Save or not","是否保存当前文件？",QMessageBox::Yes|QMessageBox::No);
        if(ok == QMessageBox::Yes){
            on_actionSave_triggered();
        }
    }

    ui->textEdit->clear();
    ui->textEdit->setReadOnly(true);
    MDfileName = nullptr;
    MDfilePath = nullptr;
    file = nullptr;
    changed = false;
}

void MDeditor::on_actionB_triggered()
{
    ui->textEdit->insertPlainText("****");
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
}

void MDeditor::on_actionI_triggered()
{
    ui->textEdit->insertPlainText("**");
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
}

void MDeditor::on_actionDownline_triggered()
{
    ui->textEdit->insertPlainText("<u></u>");
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
}

void MDeditor::on_actionDelete_triggered()
{
    ui->textEdit->insertPlainText("~~~~");
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    ui->textEdit->moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
}

void MDeditor::on_actionLink_triggered()
{
    ui->textEdit->insertPlainText("[](\"\")");
}

void MDeditor::on_actionCode_triggered()
{
    ui->textEdit->insertPlainText("\n\n```\n\n```");
   // ui->textEdit->append("```");
    ui->textEdit->moveCursor(QTextCursor::Up,QTextCursor::MoveAnchor);
}

void MDeditor::on_actionPic_triggered()
{
    pic_path = "";
    int ok = QMessageBox::question(this,"图片类型?","本地图片点击YES，否则点击NO",QMessageBox::Yes|QMessageBox::No);
    if(ok == QMessageBox::Yes)
    {
        get_pic_path();

    }
    if(ok == QMessageBox::No){
        pic_path = QInputDialog::getText(this,"图片地址","请填入图片地址",QLineEdit::Normal,"path");
        if(pic_path != "")
            ui->textEdit->insertPlainText("\n![]("+pic_path+"\"\")");
    }

}

void MDeditor::get_pic_path()
{
    QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager(this);

    connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deal(QNetworkReply*)));

    //图片地址和类型
    QString strFilePath = QFileDialog::getOpenFileName(this,"选择图片","/","*.jpg *.jepg *.png *.bmp *.gif *.svg *.psd *.ai *.TIFF *.webp *.EPS");
    QString pic_type = strFilePath.right(strFilePath.length() - strFilePath.lastIndexOf("."));

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("image/" + pic_type));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("frome-data; name=\"smfile\"; filename=\"image." + pic_type + "\""));

    QFile *file = new QFile(strFilePath);
    if(!file->open(QIODevice::ReadOnly))
    {
        qDebug()<<"image not open";
        return;
    }else{
        imagePart.setBodyDevice(file);
        multiPart->append(imagePart);
    }

    QNetworkRequest request;
    request.setUrl(QUrl("https://sm.ms/api/v2/upload"));
    networkAccessManager->post(request,multiPart);

}
void MDeditor::deal(QNetworkReply* reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug()<<"statusCode: "<<statusCode;
    if(reply->error() == QNetworkReply::NoError)
    {
        //返回的reply是json格式
        QByteArray allData = reply->readAll();
        QJsonParseError json_error;
        QJsonDocument jsonDoc(QJsonDocument::fromJson(allData, &json_error));
        qDebug()<<allData;
        if(json_error.error != QJsonParseError::NoError)
        {
            qDebug()<<"json error!";
            return;
        }
        QJsonObject rootObj = jsonDoc.object();
        if(rootObj.contains("data"))
        {
            QJsonObject subObj = rootObj.value("data").toObject();
            pic_path = subObj["url"].toString();
            qDebug()<<pic_path;
        }else{
            if(rootObj.contains("images"))
            {
                pic_path = rootObj["images"].toString();
                qDebug()<<pic_path;
            }else{
                qDebug()<<"no url!";
            }
        }
        if(pic_path != "")
            ui->textEdit->insertPlainText("\n![]("+pic_path+"\"\")");
        reply->deleteLater();
    }
}

void MDeditor::on_actionH1_triggered()
{
    ui->textEdit->insertPlainText("\n# ");
}

void MDeditor::on_actionH2_triggered()
{
    ui->textEdit->insertPlainText("\n## ");
}

void MDeditor::on_actionRefer_triggered()
{
    ui->textEdit->insertPlainText("\n> ");
}

void MDeditor::on_actionContact_triggered()
{

}

void MDeditor::on_actionList_triggered()
{

}

void MDeditor::on_actionNumList_triggered()
{

}

void MDeditor::on_action_pdf_triggered()
{
    if(file != NULL)
    {
        QDesktopServices open_pdf;
        if(!open_pdf.openUrl(file->fileName())){
            qDebug()<<"pdf 不存在";
        }
    }else{
        QMessageBox::information(this,"pdf打开失败","请先保存.md文件，然后打开");
    }
}

void MDeditor::on_actionLine_triggered()
{
    ui->textEdit->insertPlainText("\n\n---");
}

void MDeditor::on_textEdit_textChanged()
{
    //QMessageBox::information(this,"function message","in on_textEdit_textChange()");
    changed = true;
    if(!tmpFile.open(QFile::WriteOnly)){
        return;
    }
    QTextStream stream(&tmpFile);
    stream<<ui->textEdit->toPlainText();

    tmpFile.close();
}


void MDeditor::on_action_cnnct_triggered()
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
    }else{
        tcpSocket->connectToHost((QHostAddress)("192.168.1.108"),8888);
    }
}

void MDeditor::on_actionMySQL_triggered()
{
    mysql = new MySQL();

    connect(mysql,SIGNAL(send(QString)),this,SLOT(receive(QString)));
    mysql->show();
}
void MDeditor::receive(QString str)
{
    on_actionExit_triggered();
    ui->textEdit->setText(str);
    ui->textEdit->setReadOnly(false);
    changed = true;
}

void MDeditor::on_actionTex_triggered()
{
    QString tex = QInputDialog::getMultiLineText(this,"输入公式","公式");
    tex = "https://www.zhihu.com/equation?tex=" + QUrl::toPercentEncoding(tex);

    //tex.replace("+","%2B");
    //tex.replace(" ","%20");
    //tex.replace("&","%26");
    ui->textEdit->insertPlainText(QString("![](%1)").arg(tex));

}
