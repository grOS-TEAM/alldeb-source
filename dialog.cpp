#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QString parameterNama, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->btnReport->hide();
    fileSah = false;
    berhasil = false;
    debconf = false;

    ruangKerja = QDir::homePath()+"/.alldeb"; //direktori untuk penyimpanan temporer dan pengaturan alldeb
    programTar = "tar"; //perintah tar untuk mengekstrak dan melihat properti file

    if(!QDir(ruangKerja).exists()){
        QDir().mkdir(ruangKerja);
    }
    if(!QDir(ruangKerja+"/config").exists()){
        QDir().mkdir(ruangKerja+"/config");
    }
    tentangProgram = new About(this);

    QAction *aksiAbout = new QAction(QIcon::fromTheme("help-about"),tr("About"),this);
    connect(aksiAbout,SIGNAL(triggered()),this,SLOT(infoProgram()));
    QAction *aksiGuide = new QAction(QIcon::fromTheme("help-contents"),tr("Guide"),this);
    connect(aksiGuide,SIGNAL(triggered()),this,SLOT(infoPanduan()));

    //pilihan bahasa. sementara hanya dua
    QAction *indBahasa = new QAction(tr("Indonesian"),this);
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
    bahasa = QLocale::system().name();          // e.g. "id_ID"
    bahasa.truncate(bahasa.lastIndexOf('_'));   // e.g. "id"
    //qDebug() << bahasa;
    // memilih bahasa Inggris jika sistem bukan dalam locale Indonesia
    if(bahasa == "id")
    {
        indBahasa->setChecked(true);
    }
    else
    {
        engBahasa->setChecked(true);
        terjemahan.load("alldeb_en","/usr/share/alldeb/installer/lang");    //+bahasa,
        qApp->installTranslator(&terjemahan);
        ui->retranslateUi(this);
        tentangProgram->gantiBahasa();

    }

    ekstrak = new QProcess(this);
    connect(ekstrak,SIGNAL(finished(int)),this,SLOT(bacaInfoFile()));
    connect(ekstrak,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
    daftarFile = new QProcess(this);
    connect(daftarFile,SIGNAL(finished(int)),this,SLOT(bacaFile()));
    connect(daftarFile,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
    buatPaketInfo = new QProcess(this);
    connect(buatPaketInfo,SIGNAL(finished(int)),this,SLOT(bacaInfo()));
    connect(buatPaketInfo,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
    apt_get1 = new QProcess(this);
    connect(apt_get1,SIGNAL(readyRead()),this,SLOT(bacaHasilAptget()));
    connect(apt_get1,SIGNAL(finished(int)),this,SLOT(instalPaket()));
    connect(apt_get1,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
    apt_get2 = new QProcess(this);
    connect(apt_get2,SIGNAL(readyRead()),this,SLOT(bacaHasilPerintah()));
    connect(apt_get2,SIGNAL(finished(int)),this,SLOT(hapusTemporer()));
    connect(apt_get2,SIGNAL(finished(int)),this,SLOT(progresSelesai()));
    connect(apt_get2,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
    connect(ui->tempatFile,SIGNAL(textChanged(QString)),this,SLOT(memilihFile()));

    QFile polkit("/usr/bin/pkexec");        //perintah front-end untuk meminta hak administratif dengan PolicyKit
    QFile kdesudo("/usr/bin/kdesudo");      //front-end sudo di KDE
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

    //jika program dijalankan dengan "Open with..."
    if(parameterNama.count()>0)
    {
        ui->labelPilih->hide();
        ui->btnCariFile->hide();
        ui->tempatFile->hide();
        titleofWindow(parameterNama);
        //ui->tempatFile->setText(namaFile);
        bacaFileAlldeb();
    }
    //qDebug() << namaFile;
    //berkasAlldeb.setFileName(parameterNama);
    jml = 0;                        //objek untuk jumlah file deb
    ui->btnMundur->setDisabled(true);
    ui->btnInstal->setDisabled(true);
}

Dialog::~Dialog()
{

    delete ui;
}

void Dialog::on_btnCariFile_clicked()
{
    isiKotakFile = ui->tempatFile->text();
    if(isiKotakFile.isEmpty()) {
        namaFile = QFileDialog::getOpenFileName(this,tr("Select an alldeb file"),QDir::homePath(),tr("AllDeb file (*.alldeb)"));
        if(!namaFile.isNull())
        {
            bacaFileAlldeb();
            titleofWindow(namaFile);
        }
    }
    else
    {
        QFile terbuka(isiKotakFile);
        QFileInfo berkas(terbuka); //berkas saat ini
        namaFile = QFileDialog::getOpenFileName(this,tr("Select an alldeb file"),berkas.absolutePath(),tr("AllDeb file (*.alldeb)"));
        if(!namaFile.isNull())
        {
            bacaFileAlldeb();
            titleofWindow(namaFile);
            ui->labelJumlahNilai->setText("");
            ui->labelMd5Nilai->setText("");
            ui->labelUkuranNilai->setText("");
        }
    }
    ui->stackedWidget->setCurrentIndex(0);
    ui->btnInstal->setText(tr("Next"));
    ui->btnInstal->setIcon(QIcon::fromTheme("go-next"));
}

void Dialog::bacaFileAlldeb()
{
    if(!namaFile.isNull()){
        //ui->tempatFile->setText(namaFile);
        profil.setFile(namaFile);
        namaProfil = profil.completeBaseName();

        if(!QDir(ruangKerja+"/"+namaProfil).exists()){
            QDir().mkdir(ruangKerja+"/"+namaProfil);
        }

        if(QFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt").exists())
        {
            //ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));
            bacaInfoFile();
        }
        else
        {
            QStringList argumen;
            argumen << "-xzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil << "keterangan_alldeb.txt";
            ekstrak->start(programTar, argumen);
        }
        ui->tempatFile->setText(namaFile);
    }
}

//fungsi untuk mengubah ukuran file ke satuan byte (human readable)
QString Dialog::bacaUkuran(qint64 jumlah)
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

QString Dialog::bacaTeks(QString berkas,int enume)
{
    QFile namaBerkas(berkas);
    if(!berkas.isEmpty() && !enume)
    {
        if (namaBerkas.exists() && namaBerkas.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream stream(&namaBerkas);
            QString semuaisi;
            while (!stream.atEnd()){
                semuaisi = stream.readAll();

            }
            return semuaisi;
            namaBerkas.close();
            return 0;   //inilah senjata mujarab untuk menghilangkan:
                        //warning: control reaches end of non-void function [-Wreturn-type]
        }
        else
        {
            QString galat = tr("An error occured");
            return galat;
        }
    }
    else if(enume == 1)
    {
        namaBerkas.open(QIODevice::ReadOnly | QIODevice::Text);

        QTextStream stream1(&namaBerkas);
        int pos1 = enume;
        QString line;
        while (!stream1.atEnd()){
            line = stream1.readAll();
        }
        pos1 = line.indexOf('"');
        QStringRef cari(&line,pos1+1,line.lastIndexOf('"')-pos1-1);
        QString paket = cari.toString();
        return paket;
        namaBerkas.close();
        return(0);
    }
    else
    {
        QString kosong = berkas+" doesn't exist";
        return kosong;
    }
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

        daftarIsi.removeOne("keterangan_alldeb.txt");
        ui->daftarPaket->insertItems(0,daftarIsi);
        QFile filePaket(namaFile);
        qint64 ukuran = 0;
        ukuran = filePaket.size();
        QString nilai = bacaUkuran(ukuran);
        ui->labelUkuranNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+nilai+"</span></p></body></html>");

        jml = ui->daftarPaket->count();
        ui->daftarPaket->takeItem(jml-1);
        ui->labelJumlahNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+QString::number(jml-1)+
                                      "</span></p></body></html>");

        QCryptographicHash crypto(QCryptographicHash::Md5);
        filePaket.open(QFile::ReadOnly);
        while(!filePaket.atEnd()){
            crypto.addData(filePaket.readAll());
        }

        QByteArray sum = crypto.result();
        QString sums(sum.toHex());
        ui->labelMd5Nilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+sums+"</span></p></body></html>");
        filePaket.close();

        if(daftar.contains("ttf-mscorefonts-installer") || daftar.contains("mysql") || daftar.contains("phpmyadmin"))
        {
            //menangani debconf, untuk saat ini menggunakan metode ini dulu
            debconf = true;
        }
    }

    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error"));
        msgBox.setText(tr("This is not alldeb file we expected. Don't proceed, please!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        //qDebug() << "bukan file alldeb";
    }

}

void Dialog::bacaInfoFile()
{
    //ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));
    paketPaket = bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt",1);
    //QStringList paketIsi = namaPaket.split(" ");
    ui->textEdit->setHtml(tr("<html><head/><body><h2>Installation</h2><p>You are going to install following meta-package(s):</p>"
                          "<p><strong>%1</strong></p><p><br/></p><p>Always make sure that this alldeb file is from reliable source.</p></body></html>").arg(paketPaket));
    //info paling depan

    perintahAptget = "You are going to excute below command:\nsudo apt-get -o dir::etc::sourcelist="
            +ruangKerja+"/config/source_sementara.list -o dir::etc::sourceparts="+ruangKerja+"/part.d -o dir::state::lists="
            +ruangKerja+"/lists install --allow-unauthenticated -y " + paketPaket +"\n";
}

void Dialog::bacaInfo()
{
    QString output(buatPaketInfo->readAllStandardOutput());
    ui->infoPaket->appendPlainText(output);

    QStringList arg1;
    arg1 << "-u" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+"/config/source_sementara.list"
         << "-o" << "dir::etc::sourceparts="+ruangKerja+"/part.d"
         << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "update";
    apt_get1->setWorkingDirectory(ruangKerja);
    apt_get1->setProcessChannelMode(QProcess::MergedChannels);
    apt_get1->start(sandiGui,arg1,QIODevice::ReadWrite);
}

void Dialog::buatInfo()
{
    QString berkasSumber=ruangKerja+"/config/source_sementara.list";
    QFile source( berkasSumber );
    if ( source.open(QIODevice::WriteOnly) )
    {
        QTextStream stream( &source );
        stream << "deb file:"+ruangKerja+" "+namaProfil+"/" << endl;      //penyebab kegagalan
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
        //karena dependensi paket sudah menyertakan apt-utils, maka apt-ftparchive akan selalu ada
        //ini bisa dijadikan opsi
    }
    else
    {
        QMessageBox::warning(this,tr("Cannot checking"),
                             tr("Tanpa program pemindai paket, proses ini tidak bisa dilanjutkan.\n"
                                "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) atau dpkg-scan-packages"));
    }
    fileSah = false;
}

void Dialog::bacaHasilAptget()
{
    QString output(apt_get1->readAll());
    ui->infoPaket->appendPlainText(output);
    ui->progressBar->setValue(ui->progressBar->value()+1);
    //qDebug() << output;
}

void Dialog::bacaHasilPerintah()
{
    QString output(apt_get2->readAll());
    ui->infoPaket->appendPlainText(output);
    ui->progressBar->setValue(ui->progressBar->value()+1);
    if(output.contains("E:") || output.contains("Err")){
        ui->infoPaket->appendPlainText("=================\nSorry, installation not succeed.");
        prosesGagal();
        berhasil = false;
    }
    else
    {
        berhasil = true;
    }
}

void Dialog::instalPaket()
{
    QFile infoFile1(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt");

    //QStringRef cari1;
    if (infoFile1.exists() && infoFile1.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //QString paketTermuat = bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt",1);
        QStringList arg2,arg3,arg4;
        if(paketPaket.contains(" "))
        {
            arg4 << paketPaket.split(" ");
        }
        else
        {
            arg4 << paketPaket;
        }
        arg2 << "--user" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+
                "/config/source_sementara.list" << "-o" << "dir::etc::sourceparts="+ruangKerja+
                "/part.d" << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "install" <<
                "--allow-unauthenticated" << "-y"; // simulasi: << "-s"
        arg3 << "-e" << "sudo" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+
                "/config/source_sementara.list" << "-o" << "dir::etc::sourceparts="+ruangKerja+
                "/part.d" << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "install" <<
                "--allow-unauthenticated" << "-y";
        arg2.append(arg4);
        arg3.append(arg4);
        apt_get2->setWorkingDirectory(ruangKerja);
        apt_get2->setProcessChannelMode(QProcess::MergedChannels);
        QProcess *debconff = new QProcess(this);
        if(debconf == true)
        {
            debconff->setWorkingDirectory(ruangKerja);
            debconff->start("x-terminal-emulator",arg3);

            //connect(debconff,SIGNAL(finished(int)),this,SLOT(hapusTemporer()));
            //connect(debconff,SIGNAL(finished(int)),this,SLOT(progresSelesai()));
            berhasil = true;
            progresSelesai();
        }
        else
        {
            apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);

        }
        infoFile1.close();
        //qDebug() << arg2;
    }

    else
    {
        ui->infoPaket->appendPlainText(tr("Error occured"));
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

void Dialog::updateProgress()
{
    ui->progressBar->setMaximum(jml*4+10);
    ui->progressBar->setValue(jml*0.5);
}

void Dialog::prosesSelesai()
{
    ui->progressBar->hide();
}

void Dialog::progresSelesai()
{
    if(berhasil)
    {
        ui->progressBar->setValue(jml*4+10);
        if(debconf)
        {
            ui->infoPaket->appendPlainText(tr("---------------------\nProcess was relocated to terminal emulator."));
            debconf = false;
        }
        else
        {
            ui->infoPaket->appendPlainText(tr("---------------------\nProcess has been finished."));
        }
        QTimer::singleShot(2000,this,SLOT(prosesSelesai()));
    }
}

void Dialog::prosesGagal()
{

    ui->infoPaket->appendPlainText(tr("=================\nProcess failed\nPlease report this error by clicking the Report button below\n=================\nThis report was created on %1").arg(QDate::currentDate().toString("dddd, dd MMMM yyyy")));
    ui->btnReport->setHidden(false);
    ui->progressBar->setMaximum(0);
    QTimer::singleShot(3000,this,SLOT(prosesSelesai()));
}

void Dialog::on_btnInfo_clicked()
{    
    ui->btnInfo->showMenu();

}

void Dialog::memilihFile()
{
    //mengumpulkan perintah jika tombol pilih file diklik dan ui->tempaile berubah isinya
    if(!isiKotakFile.isEmpty() || !ui->btnInstal->isEnabled())
    {
        ui->btnInstal->setDisabled(false);
        //ui->btnMundur->setDisabled(false);
        //ui->btnSalinIns->setDisabled(false);
    }
}

void Dialog::infoPanduan()
{
    tentangProgram->pilihTab(0);
    tentangProgram->show();
}

void Dialog::infoProgram()
{
    tentangProgram->pilihTab(1);
    tentangProgram->show();
}

void Dialog::gantiBahasa(QAction *aksi)
{
    QString lokal = aksi->data().toString();

    terjemahan.load("alldeb_"+lokal,"/usr/share/alldeb/installer/lang");
    qApp->installTranslator(&terjemahan);

    ui->retranslateUi(this);
    tentangProgram->gantiBahasa();
}

void Dialog::on_btnKeluarProg_clicked()
{
    if(!namaFile.isEmpty() || !isiKotakFile.isEmpty())
    {
        QMessageBox tanyaHapus;
        tanyaHapus.setWindowTitle(tr("Delete cache?"));
        tanyaHapus.setText(tr("Do you want to remove temporary files in:\n")+ruangKerja+" ?");
        tanyaHapus.setIcon(QMessageBox::Question);
        QPushButton *btnYes = tanyaHapus.addButton(tr("Yes"), QMessageBox::YesRole);
        tanyaHapus.addButton(tr("No"),QMessageBox::NoRole);
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

void Dialog::on_btnInstal_clicked()
{
    int indekStak = ui->stackedWidget->currentIndex();
    int jumlah = ui->stackedWidget->count();

    if(indekStak == 0 && (ui->labelJumlahNilai->text().isEmpty() || !namaFile.isEmpty()))
    {
        QStringList variabel;
        variabel << "-tzf" << namaFile;
        daftarFile->start(programTar, variabel);
        daftarFile->setReadChannel(QProcess::StandardOutput);
        ui->btnInstal->setDisabled(false);
    }
    else if(indekStak == jumlah-2)
    {
        ui->btnInstal->setText(tr("Install"));
        ui->btnInstal->setIcon(QIcon::fromTheme("download"));
        ui->infoPaket->setPlainText(perintahAptget);
    }

    if(fileSah && ui->stackedWidget->currentIndex() == 2)
    {
        ui->progressBar->show();
        //ui->infoPaket->appendPlainText("=================");
        ekstraksi = new QProcess(this);
        QStringList argumen3;
        argumen3 << "-xvzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil
                 << "--keep-newer-files";
        // di Ubuntu 12.04, tar belum menerima argumen --skip-old-files, maka argumen ini dihapus
        ekstraksi->start(programTar, argumen3);
        connect(ekstraksi,SIGNAL(started()),this,SLOT(updateProgress()));
        connect(ekstraksi,SIGNAL(readyRead()),this,SLOT(progresEkstrak()));
        connect(ekstraksi,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
        connect(ekstraksi,SIGNAL(finished(int)),this,SLOT(buatInfo()));
    }
    else
    {
        //qDebug() << "sudah diinstal";
        //QMessageBox::warning(this,tr("Sudah diinstal"),tr("Anda sudah mengklik tombol ini."));
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
        if(!ui->btnMundur->isEnabled())
            ui->btnMundur->setEnabled(true);
    }
    //qDebug() << indekStak;
    //qDebug() << jumlah;
}

void Dialog::on_btnMundur_clicked()
{
    //int indekStak = ui->stackedWidget->currentIndex();

    if(ui->stackedWidget->currentWidget() == ui->page_2)
    {
        ui->btnMundur->setDisabled(true);
    }
    else if(ui->btnInstal->text() == tr("Install"))
    {
        ui->btnInstal->setText(tr("Next"));
        ui->btnInstal->setIcon(QIcon::fromTheme("go-next"));
    }
    //masih percobaan juga

    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);

    //qDebug() << indekStak;
}

void Dialog::titleofWindow(QString name)
{
    QFileInfo berkasAlldeb(name);
    namaFile = berkasAlldeb.absoluteFilePath();
    QString berkas = berkasAlldeb.fileName();
    //QString judul;
    if(berkas.size() > 35)
    {
        //berkas.resize(20);
        QString akhiran = berkas.right(10);
        berkas = QString("%1...%3").arg(berkas.left(20),akhiran);
    }
    //qDebug() << parameterNama;
    this->setWindowTitle("AllDebInstaller - "+berkas);
}

void Dialog::on_btnReport_clicked()
{
    QString galat = ui->infoPaket->toPlainText();

    QFile laporan(QDir::homePath()+"/alldeb-report.txt");
    //QTextStream kesalahan(&laporan);
    laporan.open(QIODevice::WriteOnly);
    laporan.write(galat.toLatin1());
    laporan.close();

    ui->infoPaket->setPlainText(tr("A report file had been created in:\n%1\nEntitled alldeb-report.txt\n\nPlease send it to author's email or to project's Bug reporting homepage in Launchpad.\nWe're sorry for this inconvenience.").arg(QDir::homePath()));
}

void Dialog::progresEkstrak()
{
    ekstraksi->readAll();
    ui->progressBar->setValue(ui->progressBar->value()+1);
}
