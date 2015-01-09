/***************************************************************************
 *   Copyright © 2014-2015 Slamet Badwi <slamet.badwi@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation version 3.                  *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

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
    polkitAgent = false;

    ruangKerja = QDir::homePath()+"/.alldeb"; //direktori untuk penyimpanan temporer dan pengaturan alldeb
    programTar = "tar"; //perintah tar untuk mengekstrak dan melihat properti file

    if(!QDir(ruangKerja).exists()){
        QDir().mkdir(ruangKerja);
    }
    if(!QDir(ruangKerja+"/config").exists()){
        QDir().mkdir(ruangKerja+"/config");
    }
    tentangProgram = new About(this);

    /**
     * @brief connect
     *
     * Connect objects
     *
     */
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

    this->cekSistem();
    this->buatMenu();

    // Jika program dijalankan dengan "Open with..."
    if(parameterNama.count()>0)
    {
        ui->labelPilih->hide();
        ui->btnCariFile->hide();
        ui->tempatFile->hide();

        QFileInfo berkasAlldeb(parameterNama);
        namaFile = berkasAlldeb.fileName();
        this->titleofWindow(namaFile);
        //ui->tempatFile->setText(namaFile);
        this->bacaFileAlldeb(parameterNama);
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

/**
 * @brief Dialog::on_btnInfo_clicked
 *
 * Slot untuk tombol bantuan dan info
 *
 */
void Dialog::on_btnInfo_clicked()
{
    ui->btnInfo->showMenu();
}

/**
 * @brief Dialog::on_btnCariFile_clicked
 *
 * Slot untuk tombol pencari file
 *
 */
void Dialog::on_btnCariFile_clicked()
{
    isiKotakFile = ui->tempatFile->text();
    QString uriFile;
    if(isiKotakFile.isEmpty()) {
        uriFile = QFileDialog::getOpenFileName(this,tr("Pilih satu file Alldeb"),
                                                QDir::homePath(),tr("File AllDeb (*.alldeb)"));
        QFileInfo infoFile(uriFile);
        if(!uriFile.isNull())
        {
            namaFile = infoFile.fileName();
            this->bacaFileAlldeb(uriFile);
            this->titleofWindow(namaFile);
        }
    }
    else
    {
        //QFile terbuka(isiKotakFile);
        QFileInfo berkas(isiKotakFile); //berkas saat ini
        uriFile = QFileDialog::getOpenFileName(this,tr("Pilih satu file Alldeb"),
                                                berkas.absolutePath(),tr("File AllDeb (*.alldeb)"));
        QFileInfo infoFile(uriFile);
        if(!uriFile.isNull())
        {
            namaFile = infoFile.fileName();
            this->bacaFileAlldeb(uriFile);
            this->titleofWindow(namaFile);
            ui->labelJumlahNilai->setText("");
            ui->labelMd5Nilai->setText("");
            ui->labelUkuranNilai->setText("");
        }
    }
    ui->stackedWidget->setCurrentIndex(0);
    ui->btnInstal->setText(tr("Lanjut"));
    ui->btnInstal->setIcon(QIcon::fromTheme("go-next"));
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
    this->close();
    //qApp->quit();
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
        variabel << "-tzf" << profil.absoluteFilePath();
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
            ui->infoPaket->setPlainText(tr("Anda akan menjalankan perintah berikut ini:"));
            ui->infoPaket->appendPlainText(perintahAptget);
            ui->infoPaket->appendPlainText("------------------------------");
        }
    }
    else if(fileSah && indekStak == 2)
    {
        ui->progressBar->show();
        //ui->infoPaket->appendPlainText("=================");
        ekstraksi = new QProcess(this);
        QStringList argumen3;
        argumen3 << "-xvzf" << profil.absoluteFilePath() << "--directory="+ruangKerja+"/"+namaProfil
                 << "--keep-newer-files";
        // di Ubuntu 12.04, tar belum menerima argumen --skip-old-files, maka argumen ini dihapus
        ekstraksi->start(programTar, argumen3);
        connect(ekstraksi,SIGNAL(started()),this,SLOT(updateProgress()));
        connect(ekstraksi,SIGNAL(readyRead()),this,SLOT(progresEkstrak()));
        connect(ekstraksi,SIGNAL(error(QProcess::ProcessError)),this,SLOT(prosesGagal()));
        connect(ekstraksi,SIGNAL(finished(int)),this,SLOT(buatInfo()));
        ui->btnInstal->setDisabled(true);
        fileSah = false;
        ui->btnKeluarProg->setDisabled(true);
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
        ui->btnInstal->setText(tr("Lanjut"));
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
    QString pesan = tr("File laporan sudah dibuat di:\n%1\nDengan nama 'alldeb-report.txt'\n"
                       "\nSilakan kirimkan ke surel pengembang atau ke laman pelaporan "
                       "Bug di Launchpad.\nMohon maaf atas ketidaknyamanan ini.")
                        .arg(QDir::homePath());
    //QTextStream kesalahan(&laporan);
    if(!laporan.exists())
    {
        laporan.open(QIODevice::WriteOnly);
        laporan.write(galat.toLatin1());
        laporan.close();

        ui->infoPaket->show();
        ui->infoPaket->setPlainText(pesan);
    }
    else
    {
        QMessageBox tanyaHapus;
        tanyaHapus.setWindowTitle(tr("Timpa file laporan?"));
        tanyaHapus.setText(tr("File laporan sudah ada.\nApakah anda ingin menimpa file laporan tersebut?"));
        tanyaHapus.setIcon(QMessageBox::Question);
        QPushButton *btnYes = tanyaHapus.addButton(tr("Ya"), QMessageBox::YesRole);
        tanyaHapus.addButton(tr("Tidak"),QMessageBox::NoRole);
        tanyaHapus.exec();
        if(tanyaHapus.clickedButton() == btnYes)
        {
            laporan.open(QIODevice::WriteOnly);
            laporan.write(galat.toLatin1());
            laporan.close();
            ui->infoPaket->show();
            ui->infoPaket->setPlainText(pesan);
        }
    }
}

/**
 * @brief Dialog::bacaFileAlldeb
 * @param QString uri
 *
 * Fungsi untuk memeriksa isi dari file Alldeb
 * yang berlokasi di uri
 * apakah berisi file keterangan_alldeb.txt
 *
 */
void Dialog::bacaFileAlldeb(QString uri)
{
    if(!uri.isNull())
    {
        //ui->tempatFile->setText(namaFile);
        profil.setFile(uri);
        namaProfil = profil.completeBaseName();

        if(!QDir(ruangKerja+"/"+namaProfil).exists()){
            QDir().mkdir(ruangKerja+"/"+namaProfil);
        }

        /*
         * Memeriksa apakah folder temporer sudah berisi keterangan_alldeb.txt
         * jika sudah, maka tidak perlu ekstrak lagi
         */
        if(QFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt").exists())
        {
            //ui->infoPaket->setPlainText(bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt"));
            bacaInfoFile();
        }
        else
        {
            QStringList argumen;
            argumen << "-xzf" << uri << "--directory="+ruangKerja+"/"+namaProfil << "keterangan_alldeb.txt";
            ekstrak->start(programTar, argumen);
        }
        ui->tempatFile->setText(uri);
    }
}

/**
 * @brief Dialog::bacaUkuran
 * @param jumlah
 * @return
 *
 * Fungsi untuk menampilkan ukuran file yang human readable.
 *
 */
QString Dialog::bacaUkuran(qint64 jumlah)
{
    float num = jumlah;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit = tr("bytes");

    while(num >= 1000.0 && i.hasNext())
    {
        unit = i.next();
        num /= 1000.0;
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
 * enume adalah parameter opsional untuk memeriksa enumerasi
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
            QString semuaIsi;
            while (!stream.atEnd()){
                semuaIsi = stream.readAll();

            }
            return semuaIsi;
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
                                "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) "
                                "atau dpkg-scan-packages"));
    }
    fileSah = false;
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
        QFile filePaket(profil.absoluteFilePath());
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
                          "<p><strong>%1</strong></p><p><br/></p><p>Selalu pastikan file ini diperoleh dari sumber yang terpercaya.</p></body></html>")
                          .arg(paketPaket));
    //info paling depan

    perintahAptget = "sudo apt-get -o Dir::Etc::Sourcelist="
            +ruangKerja+"/config/source_sementara.list -o Dir::Etc::Sourceparts="+ruangKerja+"/part.d -o Dir::State::Lists="
            +ruangKerja+"/lists install --allow-unauthenticated -y " + paketPaket;
}

/**
 * @brief Dialog::bacaInfo
 *
 * Slot untuk menampilkan info file alldeb
 *
 */
void Dialog::bacaInfo()
{
    if(sandiGui != "none" && polkitAgent)
    {
        QString output(buatPaketInfo->readAllStandardOutput());
        ui->infoPaket->appendPlainText(output);

        QStringList arg1;
        arg1 << "--disable-internal-agent" << "--user" << "root" << "apt-get"
             << "-o" << "dir::etc::sourcelist="+ruangKerja+"/config/source_sementara.list"
             << "-o" << "dir::etc::sourceparts="+ruangKerja+"/part.d"
             << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "update";
        apt_get1->setWorkingDirectory(ruangKerja);
        apt_get1->setProcessChannelMode(QProcess::MergedChannels);
        apt_get1->start("pkexec", arg1, QIODevice::ReadWrite);
    }
    else
    {
        apt_get1->start("x-terminal-emulator",QStringList() << "-e" << perintahAptget);
    }
}

/**
 * @brief Dialog::bacaHasilAptget
 *
 * Slot untuk membaca hasil perintah apt-get update
 *
 */
void Dialog::bacaHasilAptget()
{
    QString output(apt_get1->readAll());
    ui->infoPaket->appendPlainText(output.simplified());
    ui->labelStatus->setText(output);
    ui->progressBar->setValue(ui->progressBar->value()+1);
    //qDebug() << apt_get1->exitCode();
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
        QString pesanErrorApt = tr("Instalasi Gagal");
        ui->infoPaket->appendPlainText("=================");
        ui->infoPaket->appendPlainText(pesanErrorApt);
        ui->labelStatus->setText(pesanErrorApt);
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
    QFile infoFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt");

    //QStringRef cari1;
    if (infoFile.exists() && infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //QString paketTermuat = bacaTeks(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt",1);
        QStringList arg2;
        QStringList arg3;

        if(paketPaket.contains(" "))
        {
            arg3 << paketPaket.split(" ");
        }
        else
        {
            arg3 << paketPaket;
        }
        arg2 << "apt-get" << "-o" << "Dir::Etc::Sourcelist="+ruangKerja+
                "/config/source_sementara.list" << "-o" << "Dir::Etc::Sourceparts="+ruangKerja+
                "/part.d" << "-o" << "Dir::State::Lists="+ruangKerja+"/lists" << "install" <<
                "--allow-unauthenticated" << "-y"; // simulasi: << "-s"

        arg2.append(arg3);

        apt_get2->setWorkingDirectory(ruangKerja);
        apt_get2->setProcessChannelMode(QProcess::MergedChannels);
        QProcess *debconff = new QProcess(this);
        if (debconf == true || sandiGui == "none") {
            debconff->setWorkingDirectory(ruangKerja);
            arg2.prepend("-e");
            arg2.prepend("sudo");
            debconff->start("x-terminal-emulator",arg2);

            //connect(debconff,SIGNAL(finished(int)),this,SLOT(hapusTemporer()));
            //connect(debconff,SIGNAL(finished(int)),this,SLOT(progresSelesai()));
            berhasil = true;
            this->progresSelesai();
        } else if(polkitAgent) {
            if (apt_get1->exitCode() == 126 || apt_get1->exitCode() == 127) {
                this->prosesGagal();
            } else {
                arg2.prepend("root");
                arg2.prepend("--user");
                apt_get2->start("pkexec",arg2,QIODevice::ReadWrite);
                //qDebug() << arg2;
            }
        }
        else
        {
            arg2.prepend("root");
            arg2.prepend("-u");
            if(sandiGui == "kdesudo")
            {
                apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);
            }
            else
            {
                apt_get2->start(sandiGui,QStringList() << arg2.join(" "),QIODevice::ReadWrite);
            }
        }
        infoFile.close();
        //qDebug() << apt_get1->exitCode();
    }

    else
    {
        ui->infoPaket->appendPlainText(tr("Terjadi kesalahan"));
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
    ui->btnKeluarProg->setDisabled(false);
}

/**
 * @brief Dialog::prosesGagal
 *
 * Fungsi untuk dijalankan ketika proses gagal
 *
 */
void Dialog::prosesGagal()
{
    QString pesanGagal = tr("Proses instalasi gagal. Silakan laporkan kesalahan ini dengan "
                            "mengklik tombol Laporkan di bawah ini\n");
    QString tanggal = tr("Laporan dibuat pada %1")
            .arg(QDate::currentDate().toString("dddd, dd MMMM yyyy"));

    ui->infoPaket->appendPlainText("=================");
    ui->infoPaket->appendPlainText(tanggal);
    ui->labelStatus->setText(pesanGagal);
    ui->btnReport->setHidden(false);
    ui->btnMundur->setDisabled(true);
    ui->btnKeluarProg->setDisabled(false);
    ui->btnKeluarProg->setText(tr("Keluar"));
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
            QString pesanDipindah = tr("Proses dipindahkan ke terminal emulator");
            ui->infoPaket->appendPlainText("---------------------");
            ui->infoPaket->appendPlainText(pesanDipindah);
            ui->labelStatus->setText(pesanDipindah);
            debconf = false;
        }
        else
        {
            QString pesanSelesai = tr("Proses telah selesai");
            ui->infoPaket->appendPlainText("---------------------");
            ui->infoPaket->appendPlainText(pesanSelesai);
            ui->labelStatus->setText(pesanSelesai);
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

void Dialog::laporKutu()
{
    tentangProgram->pilihTab(3);
    tentangProgram->show();
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
//    QFileInfo berkasAlldeb(name);
//    namaFile = berkasAlldeb.absoluteFilePath();
//    QString berkas = berkasAlldeb.fileName();
//    QString judul;
    if(name.size() > 36)
    {
        //berkas.resize(20);
        QString akhiran = name.right(13);
        name = QString("%1...%2").arg(name.left(20),akhiran);
    }
    //qDebug() << parameterNama;
    this->setWindowTitle("AllDebInstaller - "+name);
}

/**
 * @brief Dialog::buatMenu
 *
 * Fungsi untuk membuat menu pada tombol bantuan
 *
 */
void Dialog::buatMenu()
{
    /*
     * Create actions for about button
     */
    QAction *aksiAbout = new QAction(QIcon::fromTheme("help-about"),tr("Perihal"),this);
    connect(aksiAbout,SIGNAL(triggered()),this,SLOT(infoProgram()));
    QAction *aksiGuide = new QAction(QIcon::fromTheme("help-contents"),tr("Panduan"),this);
    connect(aksiGuide,SIGNAL(triggered()),this,SLOT(infoPanduan()));
    QAction *aboutQt = new QAction(tr("Tentang Qt"),this);
    connect(aboutQt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
    QAction *laporKutu = new QAction(QIcon::fromTheme("tools-report-bug"),tr("Laporkan Kutu"),this);
    connect(laporKutu,SIGNAL(triggered()),this,SLOT(laporKutu()));

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
    btnMenu->addSeparator();
    btnMenu->addAction(laporKutu);
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


}

void Dialog::changeEvent(QEvent *ev)
{
    if(ev->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        tentangProgram->gantiBahasa();

        this->titleofWindow(namaFile);
    }
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
    // Mencegah dialog langsung ditutup
    event->ignore();

    //emit this->on_btnKeluarProg_clicked();
    if((!namaFile.isEmpty() || !isiKotakFile.isEmpty()) && !berhasil)
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

    // Menutup dialog setelah prompt konfirmasi dipilih
    event->accept();
}

/**
 * @brief Dialog::cekSistem
 *
 * Memeriksa program-program yang dibutuhkan
 * seperti PolicyKit, KdeSudo atau gksudo
 *
 */
void Dialog::cekSistem()
{
    /**
     * @brief polkit
     *
     * pkexec adalah perintah front-end untuk meminta hak administratif dengan PolicyKit
     * kdesudo adalah front-end sudo di KDE
     * gksudo adalah front-end sudo untuk GNOME
     *
     * PolicyKit diutamakan karena merupakan framework standar yang
     * ditawarkan oleh Freedesktop.org untuk distro Linux
     *
     */
    QFile pkexec("/usr/bin/pkexec");
    QFile kdesudo("/usr/bin/kdesudo");
    QFile gksudo("/usr/bin/gksudo");
    /**
     * @brief cekKDEPlasma
     * Proses untuk mencari tahu sesi desktop saat ini
     *
     *
    QProcess cekKDEPlasma;
    cekKDEPlasma.start("echo $GDMSESSION");
    cekKDEPlasma.waitForBytesWritten();
    cekKDEPlasma.waitForFinished();
    QString session = cekKDEPlasma.readAll();
    */

    if(pkexec.exists())
    {
        polkitAgent = true;
    }

    if(kdesudo.exists())
    {
        sandiGui = "kdesudo";
    }
    else if(gksudo.exists())
    {
        sandiGui = "gksudo";
    }
    else
    {
        sandiGui = "none";  //Tidak ada prompter sandi
    }

    //qDebug() << session;
}
