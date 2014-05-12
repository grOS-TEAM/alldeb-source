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
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QActionEvent>
#include <QTranslator>
#include <QLocale>
#include <QCloseEvent>
#include <QTimer>
#include <QDate>
#include "about.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QString parameterNama=0, QWidget *parent = 0);

    QString bacaUkuran(qint64 jumlah);
    QString bacaTeks(QString berkas, int enume=0);
    ~Dialog();

private slots:
    void on_btnCariFile_clicked();
    void on_btnInstal_clicked();
    void on_btnInfo_clicked();
    void on_btnKeluarProg_clicked();
    void bacaHasilPerintah();
    void bacaHasilAptget();
    void bacaInfoFile();
    void bacaFileAlldeb();
    void bacaFile();
    void bacaInfo();
    void buatInfo();
    void instalPaket();
    void memilihFile();
    void hapusTemporer();
    void gantiBahasa(QAction *aksi);
    void infoPanduan();
    void infoProgram();
    void updateProgress();
    void prosesSelesai();
    void progresSelesai();
    void prosesGagal();
    void on_btnMundur_clicked();
    void titleofWindow(QString name);
    void progresEkstrak();
    void on_btnReport_clicked();

private:
    Ui::Dialog *ui;
    QTranslator terjemahan;
    //QActionGroup *pilihBahasa;
    QString bahasa;
    QString sandiGui;
    QString namaFile;
    QString isiKotakFile;
    QString ruangKerja;
    QString namaProfil;
    QString programTar;
    QString paketPaket;
    QString perintahAptget;
    QFileInfo profil;
    QProcess *ekstrak;
    QProcess *ekstraksi;
    QProcess *daftarFile;
    QProcess *buatPaketInfo;
    QProcess *apt_get1;
    QProcess *apt_get2;    
    bool fileSah;
    bool berhasil;
    bool debconf;
    int jml;
    About *tentangProgram;
};

#endif // DIALOG_H
