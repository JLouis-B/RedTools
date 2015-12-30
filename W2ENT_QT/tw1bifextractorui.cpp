#include "tw1bifextractorui.h"
#include "ui_tw1bifextractorui.h"
#include "tw1bifextractor.h"

tw1bifextractorUI::tw1bifextractorUI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tw1bifextractorUI)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(ui->pushButton_selectFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));
}

tw1bifextractorUI::~tw1bifextractorUI()
{
    delete ui;
}

void tw1bifextractorUI::destroyWindow()
{
    delete this;
}

void tw1bifextractorUI::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "select the export folder");
    if (dir != "")
        ui->lineEdit_exportFolder->setText(dir);
}

void tw1bifextractorUI::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "select a KEY file");
    if (filename != "")
        ui->lineEdit_keyFile->setText(filename);
}

void tw1bifextractorUI::extract()
{
    const QString file = ui->lineEdit_keyFile->text();
    const QString folder = ui->lineEdit_exportFolder->text();

    QThread* thread = new QThread();
    TW1bifExtractor* extractor = new TW1bifExtractor(file, folder);
    extractor->moveToThread(thread);

    QObject::connect(thread, SIGNAL(started()), extractor, SLOT(run()));
    QObject::connect(extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));
    QObject::connect(extractor, SIGNAL(finished()), this, SLOT(extractEnd()));
    QObject::connect(extractor, SIGNAL(finished()), thread, SLOT(deleteLater()));
    QObject::connect(extractor, SIGNAL(finished()), extractor, SLOT(deleteLater()));

    ui->label_State->setText("Working...");
    thread->start();
}


void tw1bifextractorUI::extractSetProgress(int value)
{
    ui->progressBar->setValue(value);
}

void tw1bifextractorUI::extractEnd()
{
    ui->progressBar->setValue(ui->progressBar->maximum());
    ui->label_State->setText("End");
}
