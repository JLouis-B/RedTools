#ifndef GUI_EXTRACTOR_TW1_BIF_H
#define GUI_EXTRACTOR_TW1_BIF_H

#include <QDialog>
#include <QThread>

#include "Extractor_TW1_BIF.h"

namespace Ui {
class GUI_Extractor_TW1_BIF;
}

class GUI_Extractor_TW1_BIF : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TW1_BIF(QWidget *parent = nullptr);
    ~GUI_Extractor_TW1_BIF();

private:
    Ui::GUI_Extractor_TW1_BIF *_ui;
    QThread* _thread;
    Extractor_TW1_BIF* _extractor;

public slots:
    void destroyWindow();
    void selectFolder();
    void selectFile();
    void extract();

    void extractSetProgress(int value);
    void extractEnd();
    void extractFail();

    void killExtractThread();
};

#endif // GUI_EXTRACTOR_TW1_BIF_H
