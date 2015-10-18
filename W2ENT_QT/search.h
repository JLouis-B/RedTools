#ifndef SEARCH_H
#define SEARCH_H

#include <QDialog>
#include <QDir>
#include "translator.h"
#include "options.h"

namespace Ui {
class Search;
}

class Search : public QDialog
{
    Q_OBJECT

public:
    explicit Search(QWidget *parent = 0, QString language = QString());
    ~Search();


public slots:
    void search();
    void load();
    void enableButton();
    void translate(QString language);

private:
    Ui::Search *_ui;
    QString _pack0lastSearch;

    void scanFolder(QString repName, int level, std::vector<QString> keywords);

signals:
    void loadPressed(QString);
};

#endif // SEARCH_H
