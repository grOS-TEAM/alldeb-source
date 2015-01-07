#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);
    ui->setupUi(this);
    ui->labelVersiQt_2->setText(QT_VERSION_STR);
}

About::~About()
{
    delete ui;
}

void About::pilihTab(int indeks)
{
    ui->tabWidget->setCurrentIndex(indeks);
}

void About::gantiBahasa()
{
    ui->retranslateUi(this);
}
