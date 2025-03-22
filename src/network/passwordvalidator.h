#ifndef PASSWORDVALIDATOR_H
#define PASSWORDVALIDATOR_H
#include <QString>

bool checkMinLength(const QString &password);
bool containsUppercase(const QString &password);
bool containsLowercase(const QString &password);
bool containsDigit(const QString &password);
bool containsSpecialChar(const QString &password);
bool noSpacesAndCyrillic(const QString &password);

#endif // PASSWORDVALIDATOR_H
