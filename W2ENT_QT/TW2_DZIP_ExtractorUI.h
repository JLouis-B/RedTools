#ifndef TW2_DZIP_EXTRACTORUI_H
#define TW2_DZIP_EXTRACTORUI_H

#include <QDialog>
#include <QFileDialog>
#include <QtConcurrent>

#include "TW2_DZIP_Extractor.h"

namespace Ui {
class TW2_DZIP_ExtractorUI;
}

class TW2_DZIP_ExtractorUI : public QDialog
{
    Q_OBJECT

public:
    explicit TW2_DZIP_ExtractorUI(QWidget *parent = 0);
    ~TW2_DZIP_ExtractorUI();

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
    Ui::TW2_DZIP_ExtractorUI* _ui;
    QThread* _thread;
    TW2_DZIP_Extractor* _extractor;
};

#endif // TW2_DZIP_EXTRACTOR_UI_H
