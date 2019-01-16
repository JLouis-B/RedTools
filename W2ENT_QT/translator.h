#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QWidget>

class Translator
{
    static QWidget* _parent;
    static QMap<QString, QString> _trad;
    static QMap<QString, QString> loadLanguage(QString file);

public:
    static void setParentWidget(QWidget *parent);
    static QString get(QString element);
    static void loadCurrentLanguage();
};

#endif // TRANSLATOR_H
