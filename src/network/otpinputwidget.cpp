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

    // Валидатор для разрешённых символов: цифры и английские буквы любого регистра
    // Здесь используется QRegularExpression (Qt5+)
    QRegularExpression regex("[A-Za-z0-9]");
    QValidator *validator = new QRegularExpressionValidator(regex, this);

    // Создаем QLineEdit для ввода каждого символа и устанавливаем фильтр событий
    for (int i = 0; i < m_codeLength; ++i)
    {
        QLineEdit *le = new QLineEdit(this);
        le->setMaxLength(1);
        le->setAlignment(Qt::AlignCenter);
        le->setFixedSize(30, 30);

        // Устанавливаем валидатор, чтобы разрешить только цифры и английские буквы
        le->setValidator(validator);

        layout->addWidget(le);
        m_fields.append(le);

        // Переход к следующему полю при вводе
        connect(le, &QLineEdit::textChanged, this, &OTPInputWidget::handleTextChanged);

        // Устанавливаем event filter для перехвата клавиатурных событий
        le->installEventFilter(this);
    }
}
void OTPInputWidget::handleTextChanged(const QString text)
{
    QLineEdit *senderField = qobject_cast<QLineEdit*>(sender());
    if (!senderField)
        return;

    // Если введён именно один символ, переходим к следующему полю (если оно существует)
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

            // Получаем текст из буфера обмена
            QClipboard *clipboard = QApplication::clipboard();
            QString pastedText = clipboard->text().trimmed();

            // Фильтрация текста по регулярному выражению
            QRegularExpression regex("[A-Za-z0-9]");
            QString filteredText;
            for (int i = 0; i < pastedText.length(); ++i) {
                if (pastedText.mid(i, 1).contains(regex))
                    filteredText.append(pastedText.at(i));
            }
            int len = qMin(m_codeLength, filteredText.length());

            // Заполняем поля – при условии, что m_fields содержит нужное количество элементов
            for (int i = 0; i < len; ++i) {
                m_fields[i]->setText(QString(filteredText.at(i)));
            }
            if (len < m_fields.size())
                m_fields[len]->setFocus();
            else
                m_fields.last()->setFocus();

            // Возвращаем true – событие обработано.
            return true;
        }

        int index = m_fields.indexOf(field);
        if (index == -1)
            return QWidget::eventFilter(watched, event);

        // Обработка клавиши Backspace:
        // Если нажата Backspace и поле пустое, переводим фокус на предыдущее поле
        if (keyEvent->key() == Qt::Key_Backspace) {
            // Если в поле есть символ, позволяем стандартное удаление
            if (!field->text().isEmpty()) {
                return QWidget::eventFilter(watched, event);
            }
            else if (index > 0) { // Переход к предыдущему полю, если оно есть
                QLineEdit *prevField = m_fields.at(index - 1);
                prevField->setFocus();
                // При необходимости устанавливаем позицию курсора в конец
                prevField->setCursorPosition(prevField->text().length());
                return true; // событие обработано
            }
        }
        // Обработка стрелки влево:
        // Если курсор находится в начале текущего поля, переходим на предыдущее поле
        else if (keyEvent->key() == Qt::Key_Left) {
            if (field->cursorPosition() == 0 && index > 0) {
                QLineEdit *prevField = m_fields.at(index - 1);
                prevField->setFocus();
                prevField->setCursorPosition(prevField->text().length());
                return true;
            }
        }
        // Обработка стрелки вправо:
        // Если курсор находится в конце текущего поля, переходим на следующее поле
        else if (keyEvent->key() == Qt::Key_Right) {
            if (field->cursorPosition() == field->text().length() && index < m_fields.size() - 1) {
                QLineEdit *nextField = m_fields.at(index + 1);
                nextField->setFocus();
                nextField->setCursorPosition(0);
                return true;
            }
        }
    }
    // Остальные события обрабатываем стандартно
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

// Функция для сброса стиля ввода к стандартному виду
void OTPInputWidget::resetStyle()
{
    for (QLineEdit *field : m_fields)
    {
        field->setStyleSheet("");
    }
}
