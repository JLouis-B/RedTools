#include "DomHelper.h"

#include <QDomElement>
#include <QColor>
#include <QDateTime>

DomHelper::DomHelper()
{

}


void DomHelper::AppendNewStringElement(QDomDocument& dom, QDomElement& parent, QString name, QString value)
{
    QDomElement newElement = dom.createElement(name);
    parent.appendChild(newElement);
    newElement.appendChild(dom.createTextNode(value));
}

void DomHelper::AppendNewIntElement(QDomDocument& dom, QDomElement& parent, QString name, int value)
{
    AppendNewStringElement(dom, parent, name, QString::number(value));
}

void DomHelper::AppendNewDoubleElement(QDomDocument& dom, QDomElement& parent, QString name, double value)
{
    AppendNewStringElement(dom, parent, name, QString::number(value));
}

void DomHelper::AppendNewBoolElement(QDomDocument& dom, QDomElement& parent, QString name, bool value)
{
    AppendNewStringElement(dom, parent, name, QString::number(value));
}

void DomHelper::AppendNewColorElement(QDomDocument& dom, QDomElement& parent, QString name, QColor value)
{
    QDomElement newElement = dom.createElement(name);
    parent.appendChild(newElement);

    AppendNewIntElement(dom, newElement, "r", value.red());
    AppendNewIntElement(dom, newElement, "g", value.green());
    AppendNewIntElement(dom, newElement, "b", value.blue());
}

void DomHelper::AppendNewByteArrayElement(QDomDocument& dom, QDomElement& parent, QString name, QByteArray value)
{
    QString base64data = QString::fromStdString(value.toBase64().toStdString());
    AppendNewStringElement(dom, parent, name, base64data);
}

void DomHelper::AppendNewDateTimeElement(QDomDocument& dom, QDomElement& parent, QString name, QDateTime value)
{
    QString dateTimeString = value.toString();
    AppendNewStringElement(dom, parent, name, dateTimeString);
}
