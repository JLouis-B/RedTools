#include "translator.h"



QWidget* Translator::_parent = 0;

void Translator::setParentWidget(QWidget* parent)
{
    Translator::_parent = parent;
}

QString Translator::findTranslation(QString element, QString langFile)
{
    QDomDocument *dom = new QDomDocument("translation");
    QFile xml_doc(langFile);
    QString translation = "null";

    if(!xml_doc.open(QIODevice::ReadOnly))
    {
         QMessageBox::warning(_parent,"Erreur", "Erreur XML");
    }
    if (!dom->setContent(&xml_doc))
    {
        xml_doc.close();
        QMessageBox::warning(_parent, "Erreur", "Erreur XML");
    }
    QDomElement dom_element = dom->documentElement();
    QDomNode node = dom_element.firstChild();

    while(!node.isNull())
    {
        if(!dom_element.isNull() && node.nodeName() == "element")
        {
            if (element == node.firstChildElement("name").text())
            {
                delete dom;
                translation = node.firstChildElement("text").text();
                return translation;
            }
        }
        node = node.nextSibling();
    }

    xml_doc.close();
    delete dom;
}
