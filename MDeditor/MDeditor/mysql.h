#ifndef MYSQL_H
#define MYSQL_H

#include <QWidget>
#include <QSqlTableModel>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace Ui {
class MySQL;
}

class MySQL : public QWidget
{
    Q_OBJECT

public:
    explicit MySQL(QWidget *parent = nullptr);
    ~MySQL();
    QSqlTableModel *model;


private:
    Ui::MySQL *ui;

    QString hostName;
    QString dbName;
    QString userName;
    QString password;
    QSqlDatabase dbconn;


signals:
    void send(QString);
private slots:
    void on_addButton_clicked();
    void on_deleteBUtton_clicked();
    void on_openButton_clicked();
    void on_searchButton_clicked();
    //void closeEvent(QCloseEvent *event);
};

#endif // MYSQL_H
