#ifndef EXTRACTORUI_TW2_DZIP_H
#define EXTRACTORUI_TW2_DZIP_H

#include <QDialog>
#include <QThread>

#include "Extractor_TW2_DZIP.h"

namespace Ui {
class GUI_Extractor_TW2_DZIP;
}

class GUI_Extractor_TW2_DZIP : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TW2_DZIP(QWidget *parent = nullptr);
    ~GUI_Extractor_TW2_DZIP();

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
    Ui::GUI_Extractor_TW2_DZIP* _ui;
    QThread* _thread;
    Extractor_TW2_DZIP* _extractor;
};

#endif // EXTRACTORUI_TW2_DZIP_H
