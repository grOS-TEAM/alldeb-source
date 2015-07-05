Alldeb installer
================

Alldeb installer merupakan program untuk melakukan instalasi paket-paket
Ubuntu DEB yang dikemas lagi menjadi file ALLDEB. File ini dikumpulkan 
berdasarkan keterkaitan atau dependensi dari satu meta-paket atau lebih.

File alldeb dibuat dengan alldeb_maker.sh yang disertakan bersama dengan 
kode sumber ini.

Kompilasi
---------

Untuk mengompilasi kode sumber ini, anda memerlukan paket-paket untuk 
building, antara lain: cmake, build-essential, libqt4-dev, qt4-qmake.

Perintah standar untuk building adalah:

     mkdir build
     cd build
     cmake ..
     make
     sudo make install
     
Skrip cmake belum menyertakan aturan untuk uninstall, maka untuk mencopot
instalasi harus dilakukan secara manual.

     sudo rm /usr/bin/alldeb-installer
     sudo rm -rf /usr/share/alldeb
     
