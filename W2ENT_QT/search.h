#ifndef SEARCH_H
#define SEARCH_H

#include <QDialog>
#include <QDir>
#include <QThread>
#include "translator.h"
#include "options.h"

namespace Ui {
class Search;
}

class SearchEngine : public QObject
{
    Q_OBJECT

public:
    SearchEngine(QString rootDir, QStringList keywords, QStringList extensions, bool searchFolders);
    void scanFolder(QString repName, int level);

public slots:
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void sendItem(QString);

private:
    QStringList _keywords;
    QStringList _extensions;
    QString _rootDir;
    bool _searchFolders;
    bool _stopped;
};

class Search : public QDialog
{
    Q_OBJECT

public:
    explicit Search(QWidget *parent = 0);
    ~Search();


public slots:
    void search();
    void load();
    void enableButton();
    void translate();
    void destroyWindow();
    void setProgress(int progress);
    void searchEnd();
    void addResult(QString item);
    void resetExtensionsFilter();

private:
    Ui::Search *_ui;
    QThread* _thread;
    SearchEngine* _searchEngine;

    QString _rootDir;

    void killThread();

signals:
    void loadPressed(QString);
};



#endif // SEARCH_H
