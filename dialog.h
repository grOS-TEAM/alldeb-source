#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QFileSystemModel>
#include <QProcess>
#include <QByteArray>
#include <QTextStream>
#include <QCryptographicHash>
#include <QStringList>
#include <QMessageBox>
#include <QDebug>
#include <QStringRef>
#include "about.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void on_btnCariFile_clicked();

    void on_btnFolderApt_clicked();
    void on_btnSalin_clicked();
    void on_btnInstal_clicked();

    void bacaHasilPerintah();
    void bacaHasilAptget();
    void bacaInfoFile();
    void buatDaftarIsi();
    void bacaBikinInfo();
    void instalPaket();
    QString size_human(qint64 jumlah);

    void on_btnSalinIns_clicked();

    void on_pushButton_clicked();

private:
    Ui::Dialog *ui;
//    QFileDialog *namaFile;
    //QProcess *myProcess;
    QString sandiGui;
    QProcess *ekstrak;
    QProcess *daftarFile;
    QProcess *buatPaketInfo;
    QProcess *apt_get1;
    QProcess *apt_get2;
    About *tentangProgram;
};

#endif // DIALOG_H
