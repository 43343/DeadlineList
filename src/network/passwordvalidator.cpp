#include "passwordvalidator.h"
#include <QRegularExpression>

bool checkMinLength(const QString &password) {
    return password.length() >= 8;
}


bool containsUppercase(const QString &password) {
    QRegularExpression re("[A-Z]");
    return re.match(password).hasMatch();
}


bool containsLowercase(const QString &password) {
    QRegularExpression re("[a-z]");
    return re.match(password).hasMatch();
}


bool containsDigit(const QString &password) {
    QRegularExpression re("[0-9]");
    return re.match(password).hasMatch();
}


bool containsSpecialChar(const QString &password) {
    QRegularExpression re("[!@#$%^\\*]");
    return re.match(password).hasMatch();
}


bool noSpacesAndCyrillic(const QString &password) {
    QRegularExpression re("[а-яА-ЯёЁ]");
    return !re.match(password).hasMatch() && !password.contains(' ');
}
