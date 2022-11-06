#ifndef DOMHELPER_H
#define DOMHELPER_H

class QDomDocument;
class QDomElement;
class QString;
class QColor;
class QByteArray;
class QDateTime;

class DomHelper
{
public:
    DomHelper();

    static void AppendNewStringElement(QDomDocument& dom, QDomElement& parent, QString name, QString value);
    static void AppendNewIntElement(QDomDocument& dom, QDomElement& parent, QString name, int value);
    static void AppendNewDoubleElement(QDomDocument& dom, QDomElement& parent, QString name, double value);
    static void AppendNewBoolElement(QDomDocument& dom, QDomElement& parent, QString name, bool value);
    static void AppendNewColorElement(QDomDocument& dom, QDomElement& parent, QString name, QColor value);
    static void AppendNewByteArrayElement(QDomDocument& dom, QDomElement& parent, QString name, QByteArray value);
    static void AppendNewDateTimeElement(QDomDocument& dom, QDomElement& parent, QString name, QDateTime value);
};

#endif // DOMHELPER_H
