#ifndef GUI_EXTRACTOR_THECOUNCIL_H
#define GUI_EXTRACTOR_THECOUNCIL_H

#include "Extractor_TheCouncil.h"
#include <QDialog>
#include <QFileDialog>
#include <QThread>

namespace Ui {
class GUI_Extractor_TheCouncil;
}

class GUI_Extractor_TheCouncil : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TheCouncil(QWidget *parent = nullptr);
    ~GUI_Extractor_TheCouncil();

private:
    Ui::GUI_Extractor_TheCouncil *_ui;
    QThread* _thread;
    Extractor_TheCouncil* _extractor;

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

#endif // GUI_EXTRACTOR_THECOUNCIL_H
