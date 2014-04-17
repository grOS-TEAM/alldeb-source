#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->progressBar->hide();

    ekstrak = new QProcess(this);
    connect(ekstrak,SIGNAL(finished(int)),this,SLOT(bacaInfoFile()));
    daftarFile = new QProcess(this);
    connect(daftarFile,SIGNAL(finished(int)),this,SLOT(buatDaftarIsi()));
    buatPaketInfo = new QProcess(this);
    connect(buatPaketInfo,SIGNAL(finished(int)),this,SLOT(bacaBikinInfo()));
    apt_get1 = new QProcess(this);
    connect(apt_get1,SIGNAL(readyRead()),this,SLOT(bacaHasilAptget()));
    connect(apt_get1,SIGNAL(finished(int)),this,SLOT(instalPaket()));
    apt_get2 = new QProcess(this);
    connect(apt_get2,SIGNAL(readyRead()),this,SLOT(bacaHasilPerintah()));

    QFile gksudo("/usr/bin/gksudo");
    QFile kdesudo("/usr/bin/kdesudo");
    if(kdesudo.exists())
    {
        sandiGui = "kdesudo";
    }
    else if(gksudo.exists())
    {
        sandiGui = "gksudo";
    }
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_btnCariFile_clicked()
{
    QString namaFile;
    QString folderKerja = QDir::homePath()+"/.alldeb";
    if(ui->tempatFile->text().isEmpty()) {
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),QDir::homePath(),tr("File Paket (*.alldeb *.gz)"));
    }
    else
    {
        QFile fael(ui->tempatFile->text());
        QFileInfo info(fael);
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),info.absolutePath(),tr("File Paket (*.alldeb *.gz)"));

    }

    QFile filePaket(namaFile);
    QCryptographicHash crypto(QCryptographicHash::Md5);
    filePaket.open(QFile::ReadOnly);
    while(!filePaket.atEnd()){
      crypto.addData(filePaket.readAll());
    }
    QByteArray sum = crypto.result();
    QString sums(sum.toHex());
    ui->labelMd5Nilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+sums+"</span></p></body></html>");

    qint64 ukuran = 0;
    ukuran = filePaket.size();
    QString nilai = size_human(ukuran);
    ui->labelUkuranNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+nilai+"</span></p></body></html>");

    if(!QDir(folderKerja).exists()){
        QDir().mkdir(folderKerja);
    }
    ui->tempatFile->setText(namaFile);


    QFileInfo profil(namaFile);
    QString namaProfil = profil.completeBaseName();
    folderKerja += "/"+namaProfil;
    if(!QDir(folderKerja).exists()){
        QDir().mkdir(folderKerja);
    }

    if(QFile(folderKerja+"/"+namaProfil+"/keterangan_alldeb.txt").exists())
    {
        bacaInfoFile();
    }
    else
    {
    QString program = "tar";
    QStringList argumen;
    argumen << "-xzf" << ui->tempatFile->text() << "--directory="+folderKerja << "--skip-old-files" << "keterangan_alldeb.txt";
    ekstrak->start(program, argumen);
    }

    QString program2 = "tar";
    QStringList variabel;
    variabel << "-tf" << ui->tempatFile->text();
    daftarFile->start(program2, variabel);
    daftarFile->setReadChannel(QProcess::StandardOutput);


    filePaket.close();
}

QString Dialog::size_human(qint64 jumlah)
{
    float num = jumlah;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
     {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

void Dialog::bacaInfoFile()
{
    QFileInfo profil(ui->tempatFile->text());
    QString namaProfil = profil.completeBaseName();
    QFile infoFile(QDir::homePath()+"/.alldeb/"+namaProfil+"/keterangan_alldeb.txt");
    if (infoFile.exists() && infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        QTextStream stream(&infoFile);
        QString line;
        while (!stream.atEnd()){
                    line = stream.readAll();
                    ui->infoPaket->setPlainText(line+"\n");
                    //qDebug() << "linea: "<<line;
                }

      }
    else
    {
        ui->infoPaket->setPlainText(tr("Ada kesalahan"));
    }

    infoFile.close();
}

void Dialog::buatDaftarIsi()
{
    if(ui->daftarPaket->count() != 0)
    {
        ui->daftarPaket->clear();
    }
    QStringList daftarIsi;
    QString daftar(daftarFile->readAllStandardOutput());
    daftarIsi = daftar.split("\n");
    if(daftarIsi.contains("keterangan_alldeb.txt"))
    {
    daftarIsi.removeOne("keterangan_alldeb.txt");
    ui->daftarPaket->insertItems(0,daftarIsi);
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Kesalahan"));
        msgBox.setText(tr("Waduh, ini bukan file alldeb. Jangan dilanjutkan ya!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    int jml = ui->daftarPaket->count();
    ui->daftarPaket->takeItem(jml-1);
    ui->labelJumlahNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+QString::number(jml)+
                                  "</span></p></body></html>");

}

void Dialog::on_btnFolderApt_clicked()
{
    QString folderApt = QFileDialog::getExistingDirectory(this, tr("Buka folder"),
                                                           ui->tempatApt->text(),
                                                           QFileDialog::ShowDirsOnly
                                                           | QFileDialog::DontResolveSymlinks);
    ui->tempatApt->setText(folderApt);
}

void Dialog::bacaBikinInfo()
{
    QString output(buatPaketInfo->readAllStandardOutput());
    ui->infoPaket->appendPlainText(output);
    //qDebug() << output;
    QString folderKerja1 = QDir::homePath()+"/.alldeb";

    QStringList arg1;
//    arg1 << "-c" << "sudo -u "+userN+" apt-get -o dir::etc::sourcelist="+folderKerja1+
//            "/source_sementara.list -o dir::etc::sourceparts="+folderKerja1+
//            "/part.d -o dir::state::lists="+folderKerja1+"/lists update";
    arg1 << "-u" << "root" << "apt-get -o dir::etc::sourcelist="+folderKerja1+
            "/source_sementara.list -o dir::etc::sourceparts="+folderKerja1+
            "/part.d -o dir::state::lists="+folderKerja1+"/lists update";
    apt_get1->setWorkingDirectory(folderKerja1);
    apt_get1->setProcessChannelMode(QProcess::MergedChannels);
    apt_get1->start(sandiGui,arg1,QIODevice::ReadWrite);
}

void Dialog::bacaHasilAptget()
{
    QString output(apt_get1->readAll());
    ui->infoPaket->appendPlainText(output);
    //qDebug() << output;
}

void Dialog::bacaHasilPerintah()
{
    QString output(apt_get2->readAll());
    ui->infoPaket->appendPlainText(output);
}

void Dialog::on_btnInstal_clicked()
{
    QString folderKerja = QDir::homePath()+"/.alldeb";
    QFileInfo profil(ui->tempatFile->text());
    QString namaProfil = profil.completeBaseName();

    if(!ui->tempatFile->text().isEmpty())
    {
        ui->infoPaket->appendPlainText("-----------------------\n");
        QProcess *ekstraksi = new QProcess(this);
        QString program2 = "tar";
        QStringList argumen3;
        argumen3 << "-xzf" << ui->tempatFile->text() << "--directory="+folderKerja+"/"+namaProfil
                 << "--skip-old-files" << "--keep-newer-files";
        ekstraksi->start(program2, argumen3);


        QString filename=folderKerja+"/source_sementara.list";
        QFile source( filename );
        if ( source.open(QIODevice::ReadWrite) )
        {
            QTextStream stream( &source );
            stream << "deb file:"+folderKerja+"/"+namaProfil+" ./" << endl;
        }
    }
    //cek apt-ftparchive dan dpkg-scanpackages
    QFile ftpArchive("/usr/bin/apt-ftparchive");
    QFile scanPackages("/usr/bin/dpkg-scanpackages");
    if(ftpArchive.exists())
    {
        //apt-ftparchive packages . 2>/dev/null | gzip > ./Packages.gz
        QStringList arg4;
        arg4 << "-c" << "apt-ftparchive packages "+namaProfil+"/ 2>/dev/null | gzip > "+namaProfil+"/Packages.gz";

        buatPaketInfo->setWorkingDirectory(folderKerja);
        buatPaketInfo->start("sh",arg4);
        //qDebug() << argumen5;


        if(!QDir(folderKerja+"/part.d").exists()){
            QDir().mkdir(folderKerja+"/part.d");
        }
        if(!QDir(folderKerja+"/lists").exists()){
            QDir().mkdir(folderKerja+"/lists");
        }
        if(!QDir(folderKerja+"/lists/partial").exists()){
            QDir().mkdir(folderKerja+"/lists/partial");
        }


    }
    else if(scanPackages.exists())
    {

    }
    else
    {
        QMessageBox::warning(this,tr("Tidak bisa memeriksa"),
                             tr("Tanpa program pemindai paket, proses ini tidak bisa dilanjutkan.\n"
                                "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) atau dpkg-scan-packages"));
    }

}

void Dialog::on_btnSalin_clicked()
{

    QString pencarian = ui->infoPaket->toPlainText();
    int pos = pencarian.indexOf("\"");
    QStringRef cari(&pencarian,pos+1,pencarian.lastIndexOf("\"")-pos-1);
    qDebug() << cari.toString();
}

void Dialog::on_btnSalinIns_clicked()
{

}

void Dialog::on_pushButton_clicked()
{
    tentangProgram = new About(this);
    tentangProgram->show();
}

void Dialog::instalPaket()
{
    QFileInfo profil1(ui->tempatFile->text());
    QString namaProfil1 = profil1.completeBaseName();
    QFile infoFile1(QDir::homePath()+"/.alldeb/"+namaProfil1+"/keterangan_alldeb.txt");
    int pos1 = 0;
    QStringRef cari1;
    if (infoFile1.exists() && infoFile1.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        QTextStream stream(&infoFile1);
        QString line1;
        while (!stream.atEnd()){
                    line1 = stream.readAll();

                }
        pos1 = line1.indexOf("\"");
        QStringRef cari1(&line1,pos1+1,line1.lastIndexOf("\"")-pos1-1);
      }
    else
    {
        ui->infoPaket->setPlainText(tr("Ada kesalahan"));
    }


    QString folderKerja2 = QDir::homePath()+"/.alldeb";

    QStringList arg2;
    arg2 << "-u" << "root" << "apt-get -o dir::etc::sourcelist="+folderKerja2+
            "/source_sementara.list -o dir::etc::sourceparts="+folderKerja2+
            "/part.d -o dir::state::lists="+folderKerja2+"/lists install --allow-unauthenticated -y -s"
            +cari1.toString();
    apt_get2->setWorkingDirectory(folderKerja2);
    apt_get2->setProcessChannelMode(QProcess::MergedChannels);
    apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);
    infoFile1.close();
}
