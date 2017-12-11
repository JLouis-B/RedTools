#ifndef GUI_EXTRACTOR_TW3_BUNDLE_H
#define GUI_EXTRACTOR_TW3_BUNDLE_H

#include <QDialog>
#include <QFileDialog>
#include <QtConcurrent>

#include "Extractor_TW3_BUNDLE.h"

namespace Ui {
class GUI_Extractor_TW3_BUNDLE;
}

class GUI_Extractor_TW3_BUNDLE : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TW3_BUNDLE(QWidget *parent = 0);
    ~GUI_Extractor_TW3_BUNDLE();


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
    Ui::GUI_Extractor_TW3_BUNDLE *_ui;
    QThread* _thread;
    Extractor_TW3_BUNDLE* _extractor;
};

#endif // GUI_EXTRACTOR_TW3_BUNDLE_H
