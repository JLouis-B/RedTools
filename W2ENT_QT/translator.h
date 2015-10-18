#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QMainWindow>
#include <QtXml>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <iostream>

#include "settings.h"

class Translator
{
    static QWidget* _parent;
public:
    static void setParentWidget(QWidget *parent);
    static QString findTranslation(QString element, QString langFile = QString());
};

#endif // TRANSLATOR_H
