#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QString parameterNama, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    /*
     * Initialize objects
     */
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->btnReport->hide();
    ui->infoPaket->hide();
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

    /*
     * Create actions for about button
     */
    QAction *aksiAbout = new QAction(QIcon::fromTheme("help-about"),tr("Perihal"),this);
    connect(aksiAbout,SIGNAL(triggered()),this,SLOT(infoProgram()));
    QAction *aksiGuide = new QAction(QIcon::fromTheme("help-contents"),tr("Panduan"),this);
    connect(aksiGuide,SIGNAL(triggered()),this,SLOT(infoPanduan()));
    QAction *aboutQt = new QAction(tr("Tentang Qt"),this);
    connect(aboutQt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));

    // Pilihan bahasa. Sementara hanya dua.
    QAction *indBahasa = new QAction(tr("Bahasa Indonesia"),this);
    indBahasa->setCheckable(true);
    indBahasa->setData("id");
    QAction *engBahasa = new QAction(tr("Bahasa Inggris"),this);
    engBahasa->setCheckable(true);
    engBahasa->setData("en");

    /*
     * Create about button to be drop down
     */
    QMenu *btnMenu = new QMenu(this);
    btnMenu->addAction(indBahasa);
    btnMenu->addAction(engBahasa);
    btnMenu->addSeparator();

    /*
     * Add menu to the about button
     */
    QActionGroup *pilihBahasa = new QActionGroup(btnMenu);
    pilihBahasa->setExclusive(true);
    pilihBahasa->addAction(indBahasa);
    pilihBahasa->addAction(engBahasa);
    btnMenu->addAction(aksiGuide);
    btnMenu->addAction(aksiAbout);
    btnMenu->addSeparator();
    btnMenu->addAction(aboutQt);
    ui->btnInfo->setMenu(btnMenu);
    connect(pilihBahasa,SIGNAL(triggered(QAction*)),this,SLOT(gantiBahasa(QAction *)));

    // Melihat bahasa lokal sistem
    bahasa = QLocale::system().name();          // e.g. "id_ID"
    bahasa.truncate(bahasa.lastIndexOf('_'));   // e.g. "id"
    //qDebug() << bahasa;

    // Memilih bahasa Inggris jika sistem bukan dalam locale Indonesia
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

    /**
     * @brief connect
     *
     * Connect objects
     *
     */
    //connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(on_btnKeluarProg_clicked()));
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

    /**
     * @brief polkit
     *
     * pkexec adalah perintah front-end untuk meminta hak administratif dengan PolicyKit
     * kdesudo adalah front-end sudo di KDE
     *
     */
    QFile polkit("/usr/bin/pkexec");
    QFile kdesudo("/usr/bin/kdesudo");
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

    // Jika program dijalankan dengan "Open with..."
    if(parameterNama.count()>0)
    {
        ui->labelPilih->hide();
        ui->btnCariFile->hide();
        ui->tempatFile->hide();
        titleofWindow(parameterNama);
        //ui->tempatFile->setText(namaFile);
        bacaFileAlldeb();
    }
    else
    {
        ui->btnInstal->setDisabled(true);
    }
    //qDebug() << namaFile;
    //berkasAlldeb.setFileName(parameterNama);
    jml = 0;                        //objek untuk jumlah file deb
    ui->btnMundur->setDisabled(true);

}

Dialog::~Dialog()
{

    delete ui;
}

void Dialog::on_btnCariFile_clicked()
{
    isiKotakFile = ui->tempatFile->text();
    if(isiKotakFile.isEmpty()) {
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih satu file Alldeb"),QDir::homePath(),tr("File AllDeb (*.alldeb)"));
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
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih satu file Alldeb"),berkas.absolutePath(),tr("File AllDeb (*.alldeb)"));
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

/**
 * @brief Dialog::bacaFileAlldeb
 *
 * Fungsi untuk melihat isi dari file Alldeb
 *
 */
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

/**
 * @brief Dialog::bacaUkuran
 * @param jumlah
 * @return
 *
 * Fungsi untuk mengubah ukuran file ke satuan byte (human readable).
 * Ukuran pada informasi paket DEB mungkin sudah dalam byte,
 * jadi fungsi ini mungkin tidak diperlukan.
 *
 */
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

/**
 * @brief Dialog::bacaTeks
 * @param berkas
 * @param enume
 * @return
 *
 * Fungsi untuk membaca file teks
 * dari URI berkas.
 * enume adalah parameter untuk memproses enumerasi
 *
 */
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
            QString galat = tr("Terjadi kesalahan");
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
        QString kosong = berkas+" tidak ada";
        return kosong;
    }
}

/**
 * @brief Dialog::bacaFile
 *
 * Fungsi untuk memeriksa keberadaan keterangan_alldeb.txt
 *
 */
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
        msgBox.setWindowTitle(tr("Galat"));
        msgBox.setText(tr("Tampaknya ini bukanlah file Alldeb. Mohon jangan dilanjutkan!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        //qDebug() << "bukan file alldeb";
    }

}

/**
 * @brief Dialog::bacaInfoFile
 *
 * Fungsi untuk mengambil string nama paket utama yang
 * terdapat dalam file alldeb
 *
 */
void Dialog::bacaInfoFile()
{
    //ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));
    paketPaket = bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt",1);
    //QStringList paketIsi = namaPaket.split(" ");
    ui->textEdit->setHtml(tr("<html><head/><body><h2>Instalasi paket</h2><p>Anda akan menginstal meta-paket berikut ini:</p>"
                          "<p><strong>%1</strong></p><p><br/></p><p>Selalu pastikan file ini diperoleh dari sumber yang terpercaya.</p></body></html>").arg(paketPaket));
    //info paling depan

    perintahAptget = "Anda akan menjalankan perintah berikut ini:\nsudo apt-get -o dir::etc::sourcelist="
            +ruangKerja+"/config/source_sementara.list -o dir::etc::sourceparts="+ruangKerja+"/part.d -o dir::state::lists="
            +ruangKerja+"/lists install --allow-unauthenticated -y " + paketPaket +"\n";
}

/**
 * @brief Dialog::bacaInfo
 *
 * Slot untuk menampilkan info file alldeb
 *
 */
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

/**
 * @brief Dialog::buatInfo
 *
 * Fungsi untuk membuat file sources.list sementara
 * untuk digunakan apt-get update memperbarui cache
 * basis data APT
 *
 */
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
        QMessageBox::warning(this,tr("Tidak bisa memeriksa"),
                             tr("Tanpa program pemindai paket, proses ini tidak bisa dilanjutkan.\n"
                                "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) atau dpkg-scan-packages"));
    }
    fileSah = false;
}

/**
 * @brief Dialog::bacaHasilAptget
 *
 * Slot untuk membaca hasil perintah apt-get
 *
 */
void Dialog::bacaHasilAptget()
{
    QString output(apt_get1->readAll());
    ui->infoPaket->appendPlainText(output.simplified());
    ui->labelStatus->setText(output);
    ui->progressBar->setValue(ui->progressBar->value()+1);
    //qDebug() << output;
}

/**
 * @brief Dialog::bacaHasilPerintah
 *
 * Slot untuk membaca hasil perintah apt-get install
 *
 */
void Dialog::bacaHasilPerintah()
{
    QString output(apt_get2->readAll());
    ui->infoPaket->appendPlainText(output.simplified());
    if(!output.isEmpty())
    {
        ui->labelStatus->setText(output.simplified());
    }
    ui->progressBar->setValue(ui->progressBar->value()+1);
    if(output.contains("E:") || output.contains("Err")){
        ui->infoPaket->appendPlainText(tr("=================\nInstalasi gagal."));
        ui->labelStatus->setText(tr("Instalasi Gagal"));
        prosesGagal();
        berhasil = false;
    }
    else
    {
        berhasil = true;
    }
}

/**
 * @brief Dialog::instalPaket
 *
 * Fungsi untuk menjalankan apt-get install paket
 *
 */
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
        ui->infoPaket->appendPlainText(tr("Terjadi kesalahan."));
        ui->labelStatus->setText(tr("Terjadi Kesalahan"));
    }
}

/**
 * @brief Dialog::hapusTemporer
 *
 * Fungsi untuk menghapus file temporer
 * yang berada di /home/namauser/.alldeb/namafile_alldeb
 *
 */
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

/**
 * @brief Dialog::updateProgress
 *
 * Fungsi untuk memperbarui progressbar
 *
 */
void Dialog::updateProgress()
{
    ui->progressBar->setMaximum(jml*4+10);
    ui->progressBar->setValue(jml*0.5);
}

void Dialog::prosesSelesai()
{
    ui->progressBar->hide();
}

/**
 * @brief Dialog::prosesGagal
 *
 * Fungsi untuk dijalankan ketika proses gagal
 *
 */
void Dialog::prosesGagal()
{

    ui->infoPaket->appendPlainText(tr("=================\nProses gagal\nSilakan laporkan kesalahan ini dengan mengklik tombol Laporkan di bawah ini\n=================\nThis report was created on %1").arg(QDate::currentDate().toString("dddd, dd MMMM yyyy")));
    ui->labelStatus->setText(tr("Proses gagal. Silakan laporkan kesalahan ini dengan mengklik tombol Laporkan di bawah ini\nLaporan dibuat pada %1").arg(QDate::currentDate().toString("dddd, dd MMMM yyyy")));
    ui->btnReport->setHidden(false);
    ui->progressBar->setMaximum(0);
    QTimer::singleShot(3000,this,SLOT(prosesSelesai()));
}

/**
 * @brief Dialog::progresSelesai
 *
 * Fungsi untuk mengeset progressbar ke 100%
 *
 */
void Dialog::progresSelesai()
{
    if(berhasil)
    {
        ui->progressBar->setValue(jml*4+10);
        if(debconf)
        {
            ui->infoPaket->appendPlainText(tr("---------------------\nProses dipindahkan ke terminal emulator."));
            ui->labelStatus->setText(tr("Proses dipindahkan ke terminal emulator"));
            debconf = false;
        }
        else
        {
            ui->infoPaket->appendPlainText(tr("---------------------\nProses telah selesai."));
            ui->labelStatus->setText(tr("Proses telah selesai"));
        }
        QTimer::singleShot(2000,this,SLOT(prosesSelesai()));
        ui->btnKeluarProg->setText(tr("Keluar"));
        ui->btnMundur->setDisabled(true);
    }
}

void Dialog::progresEkstrak()
{
    QString output(ekstraksi->readAll());
    ui->infoPaket->appendPlainText(tr("Mengekstrak: ")+output.simplified());
    ui->labelStatus->setText(tr("Mengekstrak: ")+output.simplified());
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

void Dialog::memilihFile()
{
    //mengumpulkan perintah jika tombol pilih file diklik dan ui->tempatFile berubah isinya
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

/**
 * @brief Dialog::gantiBahasa
 * @param aksi
 *
 * Fungsi untuk mengganti bahasa aplikasi secara langsung
 * aksi adalah event penggantian bahasa
 *
 */
void Dialog::gantiBahasa(QAction *aksi)
{
    QString lokal = aksi->data().toString();

    terjemahan.load("alldeb_"+lokal,"/usr/share/alldeb/installer/lang");
    qApp->installTranslator(&terjemahan);

    ui->retranslateUi(this);
    tentangProgram->gantiBahasa();
}

void Dialog::on_btnInfo_clicked()
{
    ui->btnInfo->showMenu();

}

/**
 * @brief Dialog::on_btnKeluarProg_clicked
 *
 * Slot untuk menangani event klik pada
 * tombol Batal atau menutup program
 *
 */
void Dialog::on_btnKeluarProg_clicked()
{
    if(!namaFile.isEmpty() || !isiKotakFile.isEmpty())
    {
        QMessageBox tanyaHapus;
        tanyaHapus.setWindowTitle(tr("AllDebInstaller - Hapus tembolok?"));
        tanyaHapus.setText(tr("Apakah anda ingin menghapus berkas sementara di:\n")+ruangKerja+" ?");
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

/**
 * @brief Dialog::on_btnInstal_clicked
 *
 * Slot untuk menangani event klik pada tombol
 * Instal
 *
 */
void Dialog::on_btnInstal_clicked()
{
    int indekStak = ui->stackedWidget->currentIndex();
    //int jumlah = ui->stackedWidget->count();

    if(indekStak == 0 && (ui->labelJumlahNilai->text().isEmpty() || !namaFile.isEmpty()))
    {
        QStringList variabel;
        variabel << "-tzf" << namaFile;
        daftarFile->start(programTar, variabel);
        daftarFile->setReadChannel(QProcess::StandardOutput);
        //ui->btnInstal->setDisabled(false);
    }
    else if(indekStak == 1)
    {
        ui->btnInstal->setText(tr("Instal"));
        ui->btnInstal->setIcon(QIcon::fromTheme("download"));
        if(ui->infoPaket->toPlainText().isEmpty())
        {
            ui->infoPaket->setPlainText(perintahAptget);
        }
    }
    else if(fileSah && indekStak == 2)
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
        ui->btnInstal->setDisabled(true);
        fileSah = false;
    }

    if(!ui->btnMundur->isEnabled())
        ui->btnMundur->setEnabled(true);

    //qDebug() << indekStak;
    //qDebug() << jumlah;
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
}

/**
 * @brief Dialog::on_btnMundur_clicked
 *
 * Slot untuk menangani event klik pada tombol
 * mundur
 *
 */
void Dialog::on_btnMundur_clicked()
{
    //int indekStak = ui->stackedWidget->currentIndex();

    if(ui->stackedWidget->currentWidget() == ui->page_2)
    {
        ui->btnMundur->setDisabled(true);
    }
    else if(ui->btnInstal->text() == tr("Instal"))
    {
        ui->btnInstal->setText(tr("Maju"));
        ui->btnInstal->setIcon(QIcon::fromTheme("go-next"));
        ui->btnInstal->setDisabled(false);
    }

    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);

    //qDebug() << indekStak;
}

/**
 * @brief Dialog::on_btnReport_clicked
 *
 * Fungsi untuk membuat file laporan
 *
 */
void Dialog::on_btnReport_clicked()
{
    QString galat = ui->infoPaket->toPlainText();

    QFile laporan(QDir::homePath()+"/alldeb-report.txt");
    //QTextStream kesalahan(&laporan);
    laporan.open(QIODevice::WriteOnly);
    laporan.write(galat.toLatin1());
    laporan.close();

    ui->infoPaket->show();
    ui->infoPaket->setPlainText(tr("File laporan sudah dibuat di:\n%1\nBernama alldeb-report.txt\n\nSilakan kirimkan ke surel pengembang atau ke laman pelaporan Bug di Launchpad.\nMohon maaf atas ketidaknyamanan ini.").arg(QDir::homePath()));
}

/**
 * @brief Dialog::titleofWindow
 * @param name
 *
 * Fungsi untuk mengubah judul jendela
 * menjadi AlldebInstaller + name
 *
 */
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

/**
 * @brief Dialog::closeEvent(QCloseEvent *event)
 *
 * Slot untuk menangani closeEvent
 * yang menanyakan apakah ingin menghapus
 * berkas DEB yang diekstrak ke folder temporer
 *
 */
void Dialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    emit this->on_btnKeluarProg_clicked();
}
