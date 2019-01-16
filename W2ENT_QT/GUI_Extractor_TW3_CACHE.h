#ifndef GUI_Extractor_TW3_CACHE_H
#define GUI_Extractor_TW3_CACHE_H

#include <QDialog>
#include <QQueue>
#include <QThread>

#include "Extractor_TW3_CACHE.h"

namespace Ui {
class GUI_Extractor_TW3_CACHE;
}

class GUI_Extractor_TW3_CACHE : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Extractor_TW3_CACHE(QWidget *parent = nullptr);
    ~GUI_Extractor_TW3_CACHE();


public slots:
    void destroyWindow();
    void selectFolder();
    void selectFile();
    void extract(QString file);

    void extractSetProgress(int value);
    void extractEnd();
    void extractFail();

    void killExtractThread();

private:
    Ui::GUI_Extractor_TW3_CACHE *_ui;
    QThread* _thread;
    Extractor_TW3_CACHE* _extractor;
    QQueue<QString> _filesQueue;

    int _nbFiles;
    int _nbFilesProcessed;

    void getFiles(QString file);
    bool nextFile();
};

#endif // GUI_Extractor_TW3_CACHE_H
