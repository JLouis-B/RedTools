#ifndef EXTRACTORUI_TW2_DZIP_H
#define EXTRACTORUI_TW2_DZIP_H

#include <QDialog>
#include <QFileDialog>
#include <QtConcurrent>

#include "Extractor_TW2_DZIP.h"

namespace Ui {
class ExtractorUI_TW2_DZIP;
}

class ExtractorUI_TW2_DZIP : public QDialog
{
    Q_OBJECT

public:
    explicit ExtractorUI_TW2_DZIP(QWidget *parent = 0);
    ~ExtractorUI_TW2_DZIP();

public slots:
    void destroyWindow();
    void selectFolder();
    void selectFile();
    void extract();

    void extractSetProgress(int value);
    void extractEnd();
    void extractFail();

    void killExtractThread();

private:
    Ui::ExtractorUI_TW2_DZIP* _ui;
    QThread* _thread;
    Extractor_TW2_DZIP* _extractor;
};

#endif // EXTRACTORUI_TW2_DZIP_H
