#ifndef UTILS_THECOUNCIL_H
#define UTILS_THECOUNCIL_H

#include <QFileInfo>

enum TheCouncilFormat
{
    TheCouncil_CEF,
    TheCouncil_JSON
};

QFileInfo findFile(QString base, TheCouncilFormat format);


#endif // UTILS_THECOUNCIL_H
