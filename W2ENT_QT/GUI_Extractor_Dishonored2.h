#ifndef GUI_EXTRACTOR_DISHONORED2_H
#define GUI_EXTRACTOR_DISHONORED2_H

#include <QDialog>
#include <QThread>

#include "Extractor_Dishonored2.h"

namespace Ui {
class GUI_Extractor_Dishonored2;
}

class GUI_Extractor_Dishonored2 : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_Dishonored2(QWidget *parent = nullptr);
    ~GUI_Extractor_Dishonored2();

private:
    Ui::GUI_Extractor_Dishonored2 *_ui;
    QThread* _thread;
    Extractor_Dishonored2* _extractor;

public slots:
    void destroyWindow();
    void selectFolder();
    void selectIndexFile();
    void selectResourcesFile();
    void extract();

    void extractSetProgress(int value);
    void extractEnd();
    void extractFail();

    void killExtractThread();
};

#endif // GUI_EXTRACTOR_DISHONORED2_H
