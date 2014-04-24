#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QString parameterNama, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    fileSah = false;

    ruangKerja = QDir::homePath()+"/.alldeb"; //direktori untuk penyimpanan temporer dan pengaturan alldeb
    programTar = "tar"; //perintah tar untuk mengekstrak dan melihat properti file

    if(!QDir(ruangKerja).exists()){
        QDir().mkdir(ruangKerja);
    }
    if(!QDir(ruangKerja+"/config").exists()){
        QDir().mkdir(ruangKerja+"/config");
    }
    tentangProgram = new About(this);

    QAction *aksiAbout = new QAction(QIcon::fromTheme("help-about"),tr("Tentang"),this);
    connect(aksiAbout,SIGNAL(triggered()),this,SLOT(infoTentang()));
    QAction *aksiGuide = new QAction(QIcon::fromTheme("help-contents"),tr("Panduan"),this);
    connect(aksiGuide,SIGNAL(triggered()),this,SLOT(infoPanduan()));

    //pilihan bahasa. sementara hanya dua
    QAction *indBahasa = new QAction(tr("Indonesia"),this);
    indBahasa->setCheckable(true);
    indBahasa->setData("id");
    QAction *engBahasa = new QAction(tr("English"),this);
    engBahasa->setCheckable(true);
    engBahasa->setData("en");

    QMenu *btnMenu = new QMenu(this);
    btnMenu->addAction(indBahasa);
    btnMenu->addAction(engBahasa);
    btnMenu->addSeparator();

    QActionGroup *pilihBahasa = new QActionGroup(btnMenu);
    pilihBahasa->setExclusive(true);
    pilihBahasa->addAction(indBahasa);
    pilihBahasa->addAction(engBahasa);
    btnMenu->addAction(aksiGuide);
    btnMenu->addAction(aksiAbout);
    ui->btnInfo->setMenu(btnMenu);
    connect(pilihBahasa,SIGNAL(triggered(QAction*)),this,SLOT(gantiBahasa(QAction *)));

    // melihat bahasa lokal sistem
    bahasa = QLocale::system().name();        // e.g. "id_ID"
    bahasa.truncate(bahasa.lastIndexOf('_')); // e.g. "id"
    //qDebug() << bahasa;
    // memilih bahasa Inggris jika sistem bukan dalam locale Indonesia
    if(bahasa == "id")
    {
        indBahasa->setChecked(true);
    }
    else
    {
        engBahasa->setChecked(true);
        terjemahan.load("alldeb_en","/usr/share/alldeb/installer/lang"); //+bahasa,
        qApp->installTranslator(&terjemahan);
        ui->retranslateUi(this);
        tentangProgram->gantiBahasa();

    }

    connect(ui->tempatFile,SIGNAL(textChanged(QString)),this,SLOT(memilihFile()));
    ekstrak = new QProcess(this);
    connect(ekstrak,SIGNAL(finished(int)),this,SLOT(bacaInfoFile()));
    daftarFile = new QProcess(this);
    connect(daftarFile,SIGNAL(finished(int)),this,SLOT(bacaFile()));
    buatPaketInfo = new QProcess(this);
    connect(buatPaketInfo,SIGNAL(finished(int)),this,SLOT(bacaBikinInfo()));
    apt_get1 = new QProcess(this);
    connect(apt_get1,SIGNAL(readyRead()),this,SLOT(bacaHasilAptget()));
    connect(apt_get1,SIGNAL(finished(int)),this,SLOT(instalPaket()));
    apt_get2 = new QProcess(this);
    connect(apt_get2,SIGNAL(readyRead()),this,SLOT(bacaHasilPerintah()));
    connect(apt_get2,SIGNAL(finished(int)),this,SLOT(hapusTemporer()));
    connect(apt_get2,SIGNAL(finished(int)),this,SLOT(progresSelesai()));


    QFile polkit("/usr/bin/pkexec"); //perintah front-end untuk meminta hak administratif dengan PolicyKit
    QFile kdesudo("/usr/bin/kdesudo"); //front-end sudo di KDE
    if(polkit.exists())
    {
        sandiGui = "pkexec";
    }
    else if(kdesudo.exists())
    {
        sandiGui = "kdesudo";
    }
    else
    {
        sandiGui = "gksudo";
    }
    if(parameterNama.count()>0)
    {
        QFileInfo berkasAlldeb(parameterNama);
        namaFile = berkasAlldeb.absoluteFilePath();
        //qDebug() << parameterNama;

        //ui->tempatFile->setText(namaFile);
        bacaFileAlldeb();
    }
    //qDebug() << namaFile;
    //berkasAlldeb.setFileName(parameterNama);

}

Dialog::~Dialog()
{

    delete ui;
}

void Dialog::on_btnCariFile_clicked()
{
    isiKotakFile = ui->tempatFile->text();
    if(isiKotakFile.isEmpty()) {
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),QDir::homePath(),tr("File Paket (*.alldeb)"));
        bacaFileAlldeb();

    }
    else
    {
        QFile fael(isiKotakFile);
        QFileInfo info(fael);
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),info.absolutePath(),tr("File Paket (*.alldeb)"));

        bacaFileAlldeb();
    }

}

//fungsi untuk mengubah ukuran file ke satuan byte (human readable)
QString Dialog::size_human(qint64 jumlah)
{
    float num = jumlah;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit = tr("bytes");

    while(num >= 1024.0 && i.hasNext())
    {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

QString Dialog::bacaTeks(QString berkas)
{
    if(!berkas.isEmpty())
    {
        QFile namaBerkas(berkas);
        if (namaBerkas.exists() && namaBerkas.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream stream(&namaBerkas);
            QString baris;
            while (!stream.atEnd()){
                baris = stream.readAll();

            }
            return baris;
            namaBerkas.close();
            return 0; //inilah senjata mujarab untuk menghilangkan:
            //warning: control reaches end of non-void function [-Wreturn-type]
        }
        else
        {
            QString galat = tr("Ada kesalahan");
            return galat;
        }
    }
    else
    {
        QString kosong = berkas+" tidak ada";
        return kosong;
    }
}

void Dialog::bacaInfoFile()
{
    //    profil = ui->tempatFile->text();
    //    namaProfil = "/"+profil.completeBaseName();
    ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));
}

void Dialog::bacaFileAlldeb()
{
    if(!namaFile.isNull()){
        //ui->tempatFile->setText(namaFile);

        profil.setFile(namaFile);
        namaProfil = profil.completeBaseName();

        QStringList variabel;
        variabel << "-tf" << namaFile;
        daftarFile->start(programTar, variabel);
        daftarFile->setReadChannel(QProcess::StandardOutput);


    }
    //    else
    //    {
    //        namaFile = isiKotakFile;
    //    }
}

void Dialog::bacaFile()
{
    QStringList daftarIsi;
    QString daftar(daftarFile->readAllStandardOutput());
    daftarIsi = daftar.split("\n");
    if(daftarIsi.contains("keterangan_alldeb.txt"))
    {
        fileSah = true;
        if(ui->daftarPaket->count() != 0)
        {
            ui->daftarPaket->clear();
        }

        ui->tempatFile->setText(namaFile);
        daftarIsi.removeOne("keterangan_alldeb.txt");
        ui->daftarPaket->insertItems(0,daftarIsi);

        if(!QDir(ruangKerja+"/"+namaProfil).exists()){
            QDir().mkdir(ruangKerja+"/"+namaProfil);
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

        filePaket.close();

        if(QFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt").exists())
        {
            ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));

        }
        else
        {

            QStringList argumen;
            argumen << "-xzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil << "keterangan_alldeb.txt";
            ekstrak->start(programTar, argumen);

        }

        int jml = ui->daftarPaket->count();
        ui->daftarPaket->takeItem(jml-1);
        ui->labelJumlahNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+QString::number(jml-1)+
                                      "</span></p></body></html>");
    }

    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Kesalahan"));
        msgBox.setText(tr("Waduh, ini bukan file alldeb. Jangan dilanjutkan ya!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        //qDebug() << "bukan file alldeb";
        //ui->tempatFile->clear();
    }

}

void Dialog::on_btnFolderApt_clicked()
{
    QString folderApt = QFileDialog::getExistingDirectory(this,
                                                          tr("Pilih folder tempat file DEB berada"),
                                                          ui->tempatApt->text(),QFileDialog::ShowDirsOnly
                                                          | QFileDialog::DontResolveSymlinks);
    if(!folderApt.isNull())
        ui->tempatApt->setText(folderApt);
}

void Dialog::bacaBikinInfo()
{
    QString output(buatPaketInfo->readAllStandardOutput());
    ui->infoPaket->appendPlainText(output);
    //qDebug() << output;
    //ruangKerja = QDir::homePath()+"/.alldeb";

    QStringList arg1;
    //    arg1 << "-c" << "sudo -u "+userN+" apt-get -o dir::etc::sourcelist="+folderKerja1+
    //            "/source_sementara.list -o dir::etc::sourceparts="+folderKerja1+
    //            "/part.d -o dir::state::lists="+folderKerja1+"/lists update";
    arg1 << "-u" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+"/config/source_sementara.list"
         << "-o" << "dir::etc::sourceparts="+ruangKerja+"/part.d"
         << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "update";
    apt_get1->setWorkingDirectory(ruangKerja);
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
    if(fileSah)
    {
        ui->progressBar->show();
        ui->infoPaket->appendPlainText("-----------------------\n");
        QProcess *ekstraksi = new QProcess(this);
        QStringList argumen3;
        argumen3 << "-xzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil
                 << "--skip-old-files" << "--keep-newer-files";
        ekstraksi->start(programTar, argumen3);
        connect(ekstraksi,SIGNAL(started()),this,SLOT(updateProgress()));

        QString berkasSumber=ruangKerja+"/config/source_sementara.list";
        QFile source( berkasSumber );
        if ( source.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &source );
            stream << "deb file:"+ruangKerja+"/"+namaProfil+" ./" << endl;
        }

        //cek apt-ftparchive dan dpkg-scanpackages
        QFile ftpArchive("/usr/bin/apt-ftparchive");
        QFile scanPackages("/usr/bin/dpkg-scanpackages");
        if(ftpArchive.exists())
        {
            //apt-ftparchive packages . 2>/dev/null | gzip > ./Packages.gz
            QStringList arg4;
            arg4 << "-c" << "apt-ftparchive packages "+namaProfil+"/ 2>/dev/null | gzip > "
                    +namaProfil+"/Packages.gz";

            buatPaketInfo->setWorkingDirectory(ruangKerja);
            buatPaketInfo->start("sh",arg4);
            //qDebug() << argumen5;


            if(!QDir(ruangKerja+"/part.d").exists()){
                QDir().mkdir(ruangKerja+"/part.d");
            }
            if(!QDir(ruangKerja+"/lists").exists()){
                QDir().mkdir(ruangKerja+"/lists");
            }
            if(!QDir(ruangKerja+"/lists/partial").exists()){
                QDir().mkdir(ruangKerja+"/lists/partial");
            }


        }
        else if(scanPackages.exists())
        {
            //BELUM ADA PROSES
        }
        else
        {
            QMessageBox::warning(this,tr("Tidak bisa memeriksa"),
                                 tr("Tanpa program pemindai paket, proses ini tidak bisa dilanjutkan.\n"
                                    "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) atau dpkg-scan-packages"));
        }
        fileSah = false;
    }
    else
    {
        //qDebug() << "sudah diinstal";
        QMessageBox::warning(this,tr("Sudah diinstal"),tr("Anda sudah mengklik tombol ini."));
    }
}

void Dialog::on_btnSalin_clicked()
{
    //masih percobaan juga
    if(!namaFile.isEmpty() || !isiKotakFile.isEmpty())
    {
        ui->progressBar->show();
        if(namaFile.isEmpty())
            namaFile = isiKotakFile;

        QStringList argSalin;
        argSalin << "-u" << "root" << programTar << "-xzf" << namaFile << "--directory="+ui->tempatApt->text()
                 << "--skip-old-files" << "--keep-newer-files";
        QProcess *salin = new QProcess(this);
        salin->start(sandiGui,argSalin);

        connect(salin,SIGNAL(started()),this,SLOT(updateProgress()));
        connect(salin,SIGNAL(finished(int)),this,SLOT(progresSelesai()));
    }
}

void Dialog::updateProgress()
{
    ui->progressBar->setMaximum(10);
    ui->progressBar->setValue(5);
}

void Dialog::prosesSelesai()
{

    ui->progressBar->hide();

}

void Dialog::progresSelesai()
{
    ui->progressBar->setValue(10);
    ui->infoPaket->appendPlainText(tr("-----------------------------\nProses sudah selesai."));
    QTimer::singleShot(2000,this,SLOT(prosesSelesai()));
}

void Dialog::on_btnSalinIns_clicked()
{
    //Masih percobaan. Nanti harus diubah.

    QMessageBox::information(this,tr("Masih tahap beta"),
                             tr("Fitur ini belum ditambahkan, karena masih belum final.\nTerima kasih sudah mencoba."));
}

void Dialog::instalPaket()
{
    //    profil = ui->tempatFile->text();
    //    namaProfil = profil.completeBaseName();
    QFile infoFile1(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt");
    int pos1 = 0;
    //QStringRef cari1;
    if (infoFile1.exists() && infoFile1.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&infoFile1);
        QString line1;
        while (!stream.atEnd()){
            line1 = stream.readAll();

        }
        pos1 = line1.indexOf("\"");
        QStringRef cari1(&line1,pos1+1,line1.lastIndexOf("\"")-pos1-1);
        QString paketTermuat = cari1.toString();
        //ruangKerja = QDir::homePath()+"/.alldeb";

        //        Menemukan nama pengguna ubuntu;
        //        QStringList user(QString(QDir::homePath()).split("/"));
        //        QString userN(user.at(2));

        QStringList arg2,arg5;
        if(paketTermuat.contains(" "))
        {
            arg5 << paketTermuat.split(" ");
        }
        else
        {
            arg5 << paketTermuat;
        }
        arg2 << "--user" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+
                "/config/source_sementara.list" << "-o" << "dir::etc::sourceparts="+ruangKerja+
                "/part.d" << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "install" <<
                "--allow-unauthenticated" << "-y"; // simulasi: << "-s"
        arg2.append(arg5);
        apt_get2->setWorkingDirectory(ruangKerja);
        apt_get2->setProcessChannelMode(QProcess::MergedChannels);
        apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);
        infoFile1.close();
        //qDebug() << arg2;
    }
    else
    {
        ui->infoPaket->appendPlainText(tr("Ada kesalahan"));
    }
}

void Dialog::on_btnInfo_clicked()
{    
    ui->btnInfo->showMenu();

}

void Dialog::memilihFile()
{
    //mengumpulkan perintah jika tombol pilih file diklik dan ui->tempatFile berubah isinya
    if(!isiKotakFile.isEmpty() || !ui->btnInstal->isEnabled())
    {
        ui->btnInstal->setDisabled(false);
        ui->btnSalin->setDisabled(false);
        ui->btnSalinIns->setDisabled(false);
    }
}

void Dialog::hapusTemporer()
{
    //menghapus file-file deb yang diekstrak
    QDir dir(ruangKerja+"/"+namaProfil);
    dir.setNameFilters(QStringList() << "*.deb");
    dir.setFilter(QDir::Files);
    foreach(QString fileDeb, dir.entryList())
    {
        dir.remove(fileDeb);
    }
}

void Dialog::infoTentang()
{
    tentangProgram->pilihTab(1);
    tentangProgram->show();
}

void Dialog::infoPanduan()
{
    tentangProgram->pilihTab(0);
    tentangProgram->show();
}

void Dialog::gantiBahasa(QAction *aksi)
{
    QString lokal = aksi->data().toString();

    terjemahan.load("alldeb_"+lokal,"/usr/share/alldeb/installer/lang");//
    qApp->installTranslator(&terjemahan);

    ui->retranslateUi(this);
    tentangProgram->gantiBahasa();
}

void Dialog::on_btnKeluarProg_clicked()
{
    if(!namaFile.isEmpty() || !isiKotakFile.isEmpty())
    {
        QMessageBox tanyaHapus;
        tanyaHapus.setWindowTitle(tr("Hapus tembolok?"));
        tanyaHapus.setText(tr("Apakah anda ingin menghapus file sementara yang tersimpan di:\n")+ruangKerja+" ?");
        tanyaHapus.setIcon(QMessageBox::Question);
        QPushButton *btnYes = tanyaHapus.addButton(tr("Ya"), QMessageBox::YesRole);
        tanyaHapus.addButton(tr("Tidak"),QMessageBox::NoRole);
        tanyaHapus.exec();
        if(tanyaHapus.clickedButton() == btnYes)
        {
            QDir dir(ruangKerja+"/"+namaProfil);
            dir.setNameFilters(QStringList() << "keterangan_alldeb.txt" << "Packages.gz");
            dir.setFilter(QDir::Files);
            foreach(QString fileDeb, dir.entryList())
            {
                dir.remove(fileDeb);
            }
            QDir().rmdir(ruangKerja+"/"+namaProfil);
        }
    }
    qApp->quit();
}
