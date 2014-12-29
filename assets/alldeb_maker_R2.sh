#!/bin/bash
echo -e "\n~~ alldeb_maker ~~"
echo "Your name here"
echo "versi R-2 final"

  # Harus di-set dulu !
  # letak folder yang dijadikan profil_dir. contoh: profil_dir="/home/nif/alldeb/profil"
profil_dir="/home/"

##################
# --konfigurasi--
##################
  #1
folder_penyimpanan="/home/"
nama_keterangan="preset-name"
  # Setting untuk penyimpanan alldeb yang dibuat.
  # misalnya jika kita meminta alldeb_maker untuk membuat alldeb untuk aplikasi "gedit", maka 
  # otomatis akan disimpan di <folder_penyimpanan> dengan nama "gedit_<nama_keterangan>.alldeb"
#
  #2
apt_archives="/var/cache/apt/archives"
  # directory repository lokal Anda
#


##################
# --functions--
##################

function keluar0 {
	rm -rf "$ruangkerja"
	rm -f "$filekerja"*
	exit 0
}

function keluar1 {
	rm -rf "$ruangkerja"
	rm -f "$filekerja"*
	exit 1
}


##################
# --utama--
##################
if [ ! -d "$profil_dir" ]; then
	echo -e "\n\nDirectory profil_dir tidak ditemukan. Silahkan perbaiki setting Anda.\nSetting profil_dir Anda saat ini : \n$profil_dir\n"
	exit 1
fi
if [ `ls --hide="lock" "$profil_dir" | wc -l` -eq 0 ]; then
	echo -e "\n\nTidak ditemukan file profil di dalam directory profil_dir Anda : \n$profil_dir\nSilahkan masukkan dulu profil yang ingin dituju.\n"
	exit 1
fi
echo -e "\nMembuat file alldeb untuk profil tujuan :"
ls -1 --hide="lock" "$profil_dir"

  #step 1 : menyusun daftar paket
ruangkerja="$HOME/.ruangkerjaalldebmakerR2"
filekerja="$HOME/.alldeb_maker_list"
rm -rf "$ruangkerja"
rm -f "$filekerja"*
mkdir "$ruangkerja"
mkdir "$ruangkerja/partial"
echo -e "\nSilahkan masukkan nama aplikasi : \c"
read -r nama_aplikasi
  #untuk Ubuntu lama (misalnya 10.04) masih butuh sudo
echo -e "Memeriksa daftar dependensi..."
>"$filekerja"
semua_dependensi=`sudo apt-get --print-uris -y -o dir::state::status="$filekerja" -o dir::cache::archives="$ruangkerja" install $nama_aplikasi | grep '\.deb' | wc -l`
if [ $semua_dependensi -eq 0 ]; then
	echo -e "\nMaaf, aplikasi yang Anda inginkan tidak tersedia di Software Sources yang Anda gunakan.\n"
	keluar1
fi
echo -e "Aplikasi $nama_aplikasi memiliki $semua_dependensi paket dependensi\n"
for stat in `ls -1 --hide="lock" "$profil_dir"`; do
echo "Membaca profil : $stat"
sudo apt-get --print-uris -y -o dir::state::status="$profil_dir"/"$stat" -o dir::cache::archives=$ruangkerja install $nama_aplikasi 2> "$filekerja"-err | grep '\.deb' > "$filekerja"-tmp
cat "$filekerja"-err
if [ `cat "$filekerja"-err | wc -l` -gt 0 ]; then
	echo -e "\nMaaf, tidak dapat menyusun dependensi untuk profil : $stat\nProses dibatalkan.\n"
	keluar1
fi
jumlah_diperlukan=`cat "$filekerja"-tmp | wc -l`
jumlah_terinstall=$(( $semua_dependensi - $jumlah_diperlukan ))
cat "$filekerja"-tmp >> "$filekerja"
echo " diperlukan $jumlah_diperlukan paket ( $jumlah_terinstall paket telah terinstall )"
done

sort "$filekerja" | uniq | awk '{print $2"^"$3}' > "$filekerja"-tmp
mv "$filekerja"-tmp "$filekerja"
NUMdeb=`wc -l "$filekerja" | cut -f1 "-d "`
echo -e "\nTotal diperlukan $NUMdeb file .deb\n"
if [ $NUMdeb -eq 0 ]; then
	echo -e "Aplikasi $nama_aplikasi telah terinstall di profil yang dituju.\nFile alldeb tidak diperlukan.\n"
	keluar0
fi
sleep 1

  #step 2 : memeriksa file yang tersedia
echo -e "\n# Memerikasi ketersediaan file .deb di APT archives #\nAPT archives Anda : $apt_archives\n"
sleep 1
jml_tak_ada=0
size_download=0
for q in `cat "$filekerja"`; do
q1=`echo $q|cut -f1 -d^`
q2=`echo $q|cut -f2 -d^`
if [ `ls -1 "$apt_archives" | grep $q1 | wc -l` -gt 0 ]; then
	echo -e "\033[34m $q1 = Ada"
else
	echo -e "\033[31m $q1 = Tidak ditemukan"
	jml_tak_ada=$(( $jml_tak_ada + 1 ))
	size_download=$(( $size_download + $q2 ))
fi
done
echo -e '\e[00m'
if [ $jml_tak_ada -eq 0 ]; then
	echo -e "Semua file tersedia\n"
else
	if [ $size_download -ge 10000000000 ]; then
		size_download=$(( $size_download / 1000000000 ))
		satuan="GB"
	elif [ $size_download -ge 10000000 ]; then
		size_download=$(( $size_download / 1000000 ))
		satuan="MB"
	elif [ $size_download -ge 10000 ]; then
		size_download=$(( $size_download / 1000 ))
		satuan="kB"
	else
		satuan="Bytes"
	fi
	echo -e "\nMaaf, $jml_tak_ada file tidak tersedia. Apakah Anda bersedia melakukan download?\n(Besar download yang diperlukan : $size_download $satuan)\n"
	echo -e "  1=Ya\n  2=Tidak\n\nPilih : \c"
	read -r konfirmasi
	if [ $konfirmasi -eq 1 ]; then
		for stat in `ls -1 "$profil_dir" | grep -v "lock"`; do
		echo -e "Download-only mode : $stat"
		sudo apt-get -d -y -o dir::state::status="$profil_dir"/"$stat" -o dir::cache::archives="$apt_archives" install $nama_aplikasi
		done		

		  #cek ulang
		echo -e "\nMemeriksa ulang ketersediaan file .deb\n"
		jml_tak_ada=0
		for q1 in `cat "$filekerja"|cut -f1 -d^`; do
		if [ `ls -1 "$apt_archives" | grep $q1 | wc -l` -gt 0 ]; then
			echo -e "\033[34m $q1 = Ada"
		else
			echo -e "\033[31m $q1 = Tidak ditemukan"
			jml_tak_ada=$(( $jml_tak_ada + 1 ))
		fi
		done
		echo -e '\e[00m'
		if [ $jml_tak_ada -eq 0 ]; then
			echo -e "Semua file telah tersedia\n"
		else
			echo -e "Maaf, tidak semua file dapat di-download. Silahkan periksa koneksi ke Software Sources Anda.\nProses dibatalkan.\n"
			keluar1
		fi
	elif [ $konfirmasi -eq 2 ]; then
		echo -e "Paket yang diperlukan tidak dapat dilengkapi.\nProses dibatalkan.\n"
		keluar0
	else
		echo -e "Pilihan Anda tidak tersedia.\nProses dibatalkan.\n"
		keluar0
	fi
fi

  #step 3 : menyalin file & membuat alldeb
echo -e "# Menyusun file alldeb #"
echo -e "Memproses ... \c"
rm -rf "$ruangkerja"/partial
for q1 in `cat "$filekerja"|cut -f1 -d^`; do
	cp "$apt_archives"/$q1 "$ruangkerja"/$q1
done
if [ `ls -1 "$ruangkerja" | wc -l` -eq $NUMdeb ]; then
	versi=`ls -1 "$ruangkerja" | grep -s "$nama_aplikasi"_ | cut -f2 -d_`
	echo -e "# File alldeb #\nSatu file yang memuat file .deb beserta dependensinya.\n\nDibuat untuk aplikasi & profil :\n\n\t\"$nama_aplikasi\"\n\t(versi -$versi-)\n\n`ls -1 "$profil_dir" | grep -v "lock"`\n\nDibuat pada `date +%c`" > "$ruangkerja"/keterangan_alldeb.txt
	cd "$ruangkerja"
	tar -czf file_alldeb *
	echo -e "Selesai"
	nama_file="$nama_aplikasi"_"$nama_keterangan" 
	nama_file=`echo "$nama_file" | sed s/" "/_/g`
	echo -e "Menyimpan dengan nama : "$folder_penyimpanan"/"$nama_file".alldeb"
	if [ -e "$folder_penyimpanan"/"$nama_file".alldeb ]; then
			echo -e "--\nMaaf, nama file \""$folder_penyimpanan"/"$nama_file".alldeb\" sudah ada.\nSilahkan masukkan nama lain untuk menyimpan.\n(contoh : "$nama_file"_BARU)\n--\nNama file baru : \c"
			read -r nama_file
	fi
	mv file_alldeb "$folder_penyimpanan"/"$nama_file".alldeb
	cd - > /dev/null
	if [ ! -O "$folder_penyimpanan"/"$nama_file".alldeb ]; then
			echo -e "Penyimpanan gagal. Silahkan periksa folder penyimpanan Anda.\nSetting folder penyimpanan Anda saat ini :\n$folder_penyimpanan\n"
			keluar1
	fi
	echo -e "OK"
	echo -e "\n\n----- Laporan -----\n\nAplikasi yang diproses : $nama_aplikasi\n(versi -$versi-)\nDitujukan untuk profil :\n`ls -m --hide="lock" "$profil_dir"`\n\nSemua proses telah berhasil dilaksanakan.\nFile alldeb berhasil dibuat :\n"$folder_penyimpanan"/"$nama_file".alldeb\nsize : `du --si "$folder_penyimpanan"/"$nama_file".alldeb | cut -f 1`B\nmd5sum : `md5sum "$folder_penyimpanan"/"$nama_file".alldeb | cut -f1 "-d "`\n"
else
	echo -e "\nTerjadi kesalahan ketika penyalinan. Silahkan periksa filesystem Anda.\n"
	keluar1
fi
keluar0


##################
# --notes--
##################
##september 2013
#script ini bebas dan gratis dimanfaatkan untuk tujuan yang benar
#jika ada pertanyaan/masukan silahkan kirim email ke 
#elektronifa[at]yahoo.co.id (maaf, hanya email)
