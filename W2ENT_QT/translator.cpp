#include "Translator.h"

QWidget* Translator::_parent = 0;
QMap<QString, QString> Translator::_trad;

void Translator::setParentWidget(QWidget* parent)
{
    Translator::_parent = parent;
}

QMap<QString, QString> Translator::loadLanguage(const QString file)
{
    QDomDocument dom("translation");
    QFile xmlFile(file);

    if(!xmlFile.open(QIODevice::ReadOnly))
    {
         QMessageBox::warning(_parent, "Error", "XML error : File " + file + " can't be read");
    }
    if (!dom.setContent(&xmlFile))
    {
        xmlFile.close();
        QMessageBox::warning(_parent, "Error", "XML error : Can't create the DOM");
    }
    QDomElement domElement = dom.documentElement();
    QDomNode node = domElement.firstChild();

    QMap<QString, QString> translation;
    while(!node.isNull())
    {
        if(!domElement.isNull() && node.nodeName() == "element")
        {
            const QDomElement nodeElement = node.toElement();
            translation.insert(nodeElement.attribute("id"), nodeElement.attribute("text"));
        }
        node = node.nextSibling();
    }
    xmlFile.close();

    return translation;
}

void Translator::loadCurrentLanguage()
{
    const QString file = QCoreApplication::applicationDirPath() + "/" + Settings::_language;
    _trad = loadLanguage(file);
}

QString Translator::get(QString element)
{
    const QMap<QString, QString>::iterator it = _trad.find(element);
    if (it == _trad.end())
        return "null";
    else
        return it.value();
}
