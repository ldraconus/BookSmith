#ifndef UTIL_H
#define UTIL_H

#include <QMessageBox>

class Util
{
public:
    Util();

    static int YesNo(const char* question);
    static int YesNoCancel(const char* question);
    static int OK(const char* question);
    static int OKCancel(const char* question);

    static int YesNo(const QString& question);
    static int YesNoCancel(const QString& question);
    static int OK(const QString& question);
    static int OKCancel(const QString& question);
};

#endif // UTIL_H
