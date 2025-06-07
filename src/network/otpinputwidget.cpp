#include "otpinputwidget.h"
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QApplication>
#include <QClipboard>
#include <QEvent>

OTPInputWidget::OTPInputWidget(QWidget *parent, int codeLength)
    : QWidget(parent), m_codeLength(codeLength)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(5);
    layout->setContentsMargins(0, 0, 0, 0);

    QRegularExpression regex("[A-Za-z0-9]");
    QValidator *validator = new QRegularExpressionValidator(regex, this);

    for (int i = 0; i < m_codeLength; ++i)
    {
        QLineEdit *le = new QLineEdit(this);
        le->setMaxLength(1);
        le->setAlignment(Qt::AlignCenter);
        le->setFixedSize(30, 30);

        le->setValidator(validator);

        layout->addWidget(le);
        m_fields.append(le);

        connect(le, &QLineEdit::textChanged, this, &OTPInputWidget::handleTextChanged);

        le->installEventFilter(this);
    }
}
void OTPInputWidget::handleTextChanged(const QString text)
{
    QLineEdit *senderField = qobject_cast<QLineEdit*>(sender());
    if (!senderField)
        return;

    if (text.length() == 1) {
        int index = m_fields.indexOf(senderField);
        if (index != -1 && index < m_codeLength - 1) {
            m_fields[index + 1]->setFocus();
        }
    }
}
bool OTPInputWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        QLineEdit *field = qobject_cast<QLineEdit*>(watched);
        if (!field)
            return QWidget::eventFilter(watched, event);
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_V) {
            QLineEdit *field = qobject_cast<QLineEdit*>(watched);
            if (!field)
                return QWidget::eventFilter(watched, event);

            QClipboard *clipboard = QApplication::clipboard();
            QString pastedText = clipboard->text().trimmed();

            QRegularExpression regex("[A-Za-z0-9]");
            QString filteredText;
            for (int i = 0; i < pastedText.length(); ++i) {
                if (pastedText.mid(i, 1).contains(regex))
                    filteredText.append(pastedText.at(i));
            }
            int len = qMin(m_codeLength, filteredText.length());

            for (int i = 0; i < len; ++i) {
                m_fields[i]->setText(QString(filteredText.at(i)));
            }
            if (len < m_fields.size())
                m_fields[len]->setFocus();
            else
                m_fields.last()->setFocus();

            return true;
        }

        int index = m_fields.indexOf(field);
        if (index == -1)
            return QWidget::eventFilter(watched, event);

        if (keyEvent->key() == Qt::Key_Backspace) {
            if (!field->text().isEmpty()) {
                return QWidget::eventFilter(watched, event);
            }
            else if (index > 0) {
                QLineEdit *prevField = m_fields.at(index - 1);
                prevField->setFocus();
                prevField->setCursorPosition(prevField->text().length());
                return true;
            }
        }
        else if (keyEvent->key() == Qt::Key_Left) {
            if (field->cursorPosition() == 0 && index > 0) {
                QLineEdit *prevField = m_fields.at(index - 1);
                prevField->setFocus();
                prevField->setCursorPosition(prevField->text().length());
                return true;
            }
        }
        else if (keyEvent->key() == Qt::Key_Right) {
            if (field->cursorPosition() == field->text().length() && index < m_fields.size() - 1) {
                QLineEdit *nextField = m_fields.at(index + 1);
                nextField->setFocus();
                nextField->setCursorPosition(0);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

QString OTPInputWidget::code() const
{
    QString res;
    for (auto field : m_fields)
        res.append(field->text());
    return res;
}
void OTPInputWidget::applyErrorStyle()
{
    for (QLineEdit *field : m_fields)
    {
        if (field->text().isEmpty())
        {
            field->setStyleSheet("border: 1px solid red;");
        }
    }
}
void OTPInputWidget::applyErrorStyleAll()
{
    for (QLineEdit *field : m_fields)
    {
        field->setStyleSheet("border: 1px solid red;");
    }
}
void OTPInputWidget::setEnabled(const bool& enabled)
{
    for (QLineEdit *field : m_fields)
    {
        field->setEnabled(enabled);
    }
}

void OTPInputWidget::resetStyle()
{
    for (QLineEdit *field : m_fields)
    {
        field->setStyleSheet("");
    }
}
