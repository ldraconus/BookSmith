#ifndef UTIL_H
#define UTIL_H

#include <QMessageBox>

class Util
{
public:
    Util();

    static int YesNo(const char* question, const char* title = nullptr);
    static int YesNoCancel(const char* question, const char* title = nullptr);
    static int OK(const char* question, const char* title = nullptr);
    static int OKCancel(const char* question, const char* title = nullptr);

    static int YesNo(const QString& question, const QString& title = "");
    static int YesNoCancel(const QString& question, const QString& title = "");
    static int OK(const QString& question, const QString& title = "");
    static int OKCancel(const QString& question, const QString& title = "");
};

#endif // UTIL_H
