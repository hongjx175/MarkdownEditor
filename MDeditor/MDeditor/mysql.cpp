#include "mysql.h"
#include "ui_mysql.h"
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QSqlRecord>

MySQL::MySQL(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MySQL)
{
    ui->setupUi(this);
    model = new QSqlTableModel(ui->tableView);

    hostName = "192.168.1.108";
    dbName = "markdown";
    userName = "root";
    password = "password";

    dbconn = QSqlDatabase::addDatabase("QMYSQL");
    dbconn.setHostName(hostName);
    dbconn.setPort(3306);
    dbconn.setDatabaseName(dbName);
    dbconn.setUserName(userName);
    dbconn.setPassword(password);

    qDebug("database open status: %d\n",dbconn.open());
    //QSqlError sql_error = dbconn.lastError();
    model = new QSqlTableModel(this);
    model->setTable("markdowntable");
    model->select();
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("MDfileName"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("MDfileContent"));
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

}

MySQL::~MySQL()
{
    dbconn.close();
    delete ui;
}

void MySQL::on_addButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,"选择文件","/","*.md");
    QFile *file = new QFile(path);
    file->open(QIODevice::ReadOnly);

    QSqlRecord record = model->record();
    record.setValue("mdname",file->fileName().split("/").last());
    record.setValue("mddata",file->readAll());
    file->close();
    int row = model->rowCount();
    model->insertRecord(row,record);
    model->submit();
    delete file;
}

void MySQL::on_deleteBUtton_clicked()
{
    int select_row = ui->tableView->currentIndex().row();
    model->removeRow(select_row);
    int ok = QMessageBox::information(this,"确认删除🏇?","确认删除🏇?",QMessageBox::Yes|QMessageBox::No);
    if(ok == QMessageBox::Yes)
    {
        model->submit();
        model->select();
        ui->tableView->setModel(model);
    }else{
        model->revert();
    }
}

void MySQL::on_openButton_clicked()
{
    int select_row = ui->tableView->currentIndex().row();
    QString str = model->data(model->index(select_row,1)).toString();
    emit send(str);
}

void MySQL::on_searchButton_clicked()
{
    int rowCount = model->rowCount();
    for(int i = 0;i < rowCount;i++)
    {
           ui->tableView->showRow(i);
    }
    QString filename = ui->lineEdit->text();
    if(filename != ""){
        int rowCount = model->rowCount();
        for(int i = 0;i < rowCount;i++)
        {
            if(!model->data(model->index(i,0)).toString().contains(filename))
               ui->tableView->hideRow(i);
        }
    }
}

