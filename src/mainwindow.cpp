#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAction>
#include <QIcon>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QDialog>
#include <QDateTime>
#include <QUrl>
#include <QFile>
#include <QMessageBox>
#include <QUuid>
#include <QTranslator>
#include "tasks/edittask.h"
#include "settings.h"
#include "binarydatahandler.h"
#include "network/authorizationform.h"
#include "network/registrationform.h"
#include "network/changepasswordform.h"
#include "network/securestorage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , trayIcon(new QSystemTrayIcon(this))
    , trayMenu(new QMenu(this))
    , userRegisterMenu(new QMenu(this))
    , userMenu(new QMenu(this))
    , updateTimer(new QTimer(this))
    , soundEffect(new QSoundEffect(this))
    , taskList(new QList<TaskForm*>)
    , config(new Config())
{
    loadFromFile("config", config);
    QTranslator *translator = new QTranslator();
    QString translationFile;
    if(config->language == "ru")
    {
        translationFile = "translations/deadlinelist_ru.qm";
    }
    else
    {
        config->language = "en";
        translationFile = "translations/deadlinelist_en.qm";
    }
    if (!translator->load(translationFile)) {
        qWarning() << "Failed to load translation file:" << translationFile;

        // Дополнительная диагностика
        if (!QFile::exists(translationFile)) {
            qDebug() << "File does not exist in resources";
        } else {
            qDebug() << "File exists but could not be loaded";
            QFile file(translationFile);
            if (file.open(QIODevice::ReadOnly)) {
                qDebug() << "File size:" << file.size() << "bytes";
                file.close();
            }
        }

        delete translator;
        return;
    }

    qApp->installTranslator(translator);
    ui->setupUi(this);
    QStringList args = QCoreApplication::arguments();
    if (!(args.contains("--autostart") && config->launchingTray)) {
        this->show();
    }

    if(loadFromFile("tasks", taskList, ui->scrollAreaWidgetContents))
    {
        QDateTime currentTime = QDateTime::currentDateTime();
        for(TaskForm* task : *taskList)
        {
            connect(task, &TaskForm::taskDeleted, this, &MainWindow::onTaskDeleted);
            connect(task, &TaskForm::taskEdited, this, &MainWindow::onTaskEdited);
            QDateTime deadline = task->getDeadlineDateTime();
            if(currentTime.secsTo(deadline) > 0 && currentTime.secsTo(deadline) <= 3600)
            {
                notifiedEndingSoon.insert(task);
            }
            else if(currentTime >= deadline)
            {
                notifiedEndingSoon.insert(task);
                notifiedMissedDeadline.insert(task);
            }
        }
    }

    trayIcon->setIcon(QIcon(":/icons/icons/iconApp.ico"));

    openAction = new QAction(tr("Open"), this);
    quitAction = new QAction(tr("Exit"), this);

    connect(openAction, &QAction::triggered, this, &MainWindow::showWindow);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);

    trayMenu->addAction(openAction);
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

    loginUserMenu = new QAction(tr("Log in"), userRegisterMenu);
    registrationUserMenu = new QAction(tr("Register"), userRegisterMenu);
    connect(loginUserMenu, &QAction::triggered, this, &MainWindow::onLogin);
    connect(registrationUserMenu, &QAction::triggered, this, &MainWindow::onRegistration);
    userRegisterMenu->addAction(loginUserMenu);
    userRegisterMenu->addAction(registrationUserMenu);
    connect(ui->userButton, &QPushButton::clicked, this, &MainWindow::showUserMenu);
    connect(ui->warningButton, &QPushButton::clicked, this, &MainWindow::showWarning);

    changePasswordUserMenu = new QAction(tr("Change password"), userRegisterMenu);
    exitUserMenu = new QAction(tr("Log out"), userRegisterMenu);
    connect(changePasswordUserMenu, &QAction::triggered, this, &MainWindow::onChangePassword);
    connect(exitUserMenu, &QAction::triggered, this, &MainWindow::onExit);
    userMenu->addAction(changePasswordUserMenu);
    userMenu->addAction(exitUserMenu);

    ui->allTasks->setStyleSheet("QPushButton { background-color: none; border: 3px solid rgb(133, 139, 225) ; font-size:40px; }");
    ui->completingTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->deadlineEnded->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completedTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");

    connect(ui->allTasks, &QPushButton::clicked, this, &MainWindow::showAllTasks);
    connect(ui->completingTasks, &QPushButton::clicked, this, &MainWindow::showEndingSoonTasks);
    connect(ui->deadlineEnded, &QPushButton::clicked, this, &MainWindow::showMissedDeadlineTasks);
    connect(ui->completedTasks, &QPushButton::clicked, this, &MainWindow::showCompletedTasks);

    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsButtonClicked);

    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateTaskVisibility);
    updateTimer->start(2000);

    trayIcon->show();
    SecureStorage* secureStorage = new SecureStorage("deadlinelist", this);

    m_socket = new SocketClient("192.168.0.217", secureStorage, 1234, taskList, this);
    qDebug() << QString::fromUtf8(secureStorage->load("user"));
    m_socket->setUserId(secureStorage->load("user"));
    m_socket->setToken(secureStorage->load("token"));
    connect(m_socket, &SocketClient::connected, this, &MainWindow::hideButtonWarning);
    connect(m_socket, &SocketClient::disconnected, this, &MainWindow::showButtonWarning);
    connect(m_socket, &SocketClient::validSession, this, &MainWindow::hideButtonWarning);
    connect(m_socket, &SocketClient::invalidSession, this, &MainWindow::showButtonWarning);
    connect(m_socket, &SocketClient::exitSuccessfully, this, &MainWindow::showButtonWarning);
    connect(m_socket, &SocketClient::newTaskSynced, this, [&, this] (TaskForm* newTaskSynced)
            {
                connect(newTaskSynced, &TaskForm::taskDeleted, this, &MainWindow::onTaskDeleted);
                connect(newTaskSynced, &TaskForm::taskEdited, this, &MainWindow::onTaskEdited);
    });

    updateTaskVisibility();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    this->hide();

    trayIcon->showMessage(
        tr("The application is minimized"),
        tr("The app has been minimized to the tray. Click on the icon to open it."),
        QSystemTrayIcon::Information,
        2000
        );
}

void MainWindow::showWindow() {
    if (this->isVisible()) {
        this->raise();
        this->activateWindow();
        return;
    }
    QRect startGeometry = QRect(this->x(), this->y(), 100, 100);
    QRect endGeometry = this->geometry();

    this->setGeometry(startGeometry);
    this->show();

    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(500);
    animation->setStartValue(startGeometry);
    animation->setEndValue(endGeometry);
    animation->setEasingCurve(QEasingCurve::OutBounce);

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
void MainWindow::showUserMenu() {
    if(m_socket->getConnected())
    {
        QPoint pos = ui->userButton->mapToGlobal(QPoint(0, ui->userButton->height()));
        if(!m_socket->getStatusAuthorization())
        {
            userRegisterMenu->exec(pos);
        }
        else
        {
            userMenu->exec(pos);
        }
    }
}
void MainWindow::onLogin() {
    AuthorizationForm autorizationForm(m_socket, this);
    autorizationForm.exec();
}
void MainWindow::onRegistration()
{
    RegistrationForm registrationForm(m_socket, this);
    registrationForm.exec();
}
void MainWindow::onExit()
{
    m_socket->exitUser();
}
void MainWindow::onChangePassword()
{
    ChangePasswordForm changePasswordForm(m_socket, this);
    changePasswordForm.exec();
}
void MainWindow::showWarning()
{
    if(!m_socket->getConnected())
        QMessageBox::warning(nullptr, tr("Houston, we have a problem"),
                             tr("There is no connection to the server."));
    else if(!m_socket->getStatusAuthorization())
        QMessageBox::warning(nullptr, tr("Houston, we have opportunities"),
                             tr("Log in to your account to save tasks between devices."));
}
void MainWindow::onAddButtonClicked()
{
    EditTask addTask(this);
    addTask.setWindowTitle("Add task");
    addTask.setButtonAcceptText(tr("Add"));
    TaskForm *newTask = new TaskForm(ui->scrollAreaWidgetContents);
    QDateTime currentTime = QDateTime::currentDateTime();
    //QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());

    switch (addTask.exec()) {
    case QDialog::Accepted:
        qDebug() << "Accepted";
        newTask->setParameters(addTask.getTask(), addTask.getDeadlineTime(), addTask.getDeadlineDateTime(), QDateTime::currentDateTime(), QUuid::createUuid().toString());
        //layout->addWidget(newTask);
        taskList->append(newTask);
        connect(newTask, &TaskForm::taskDeleted, this, &MainWindow::onTaskDeleted);
        connect(newTask, &TaskForm::taskEdited, this, &MainWindow::onTaskEdited);
        if(currentTime.secsTo(newTask->getDeadlineDateTime()) > 0 && currentTime.secsTo(newTask->getDeadlineDateTime()) <= 3600)
        {
            notifiedEndingSoon.insert(newTask);
        }
        else if(currentTime >= newTask->getDeadlineDateTime())
        {
            notifiedEndingSoon.insert(newTask);
            notifiedMissedDeadline.insert(newTask);
        }
        overwritingFile("tasks", taskList);
        updateTaskVisibility();
        m_socket->syncTasks({newTask});
        break;
    case QDialog::Rejected:
        qDebug() << "Rejected";
        delete newTask;
        break;
    default:
        delete newTask;
        qDebug() << "Unexpected";
    }
}

void MainWindow::onTaskDeleted(TaskForm *task, DeletedTaskData deletedTask) {
    taskList->removeOne(task); // Remove from list
    if (notifiedMissedDeadline.contains(task)) {
        notifiedMissedDeadline.remove(task);
    }
    if (notifiedEndingSoon.contains(task)) {
        notifiedEndingSoon.remove(task);
    }
    for(TaskForm* task : *taskList)
    {
        qDebug() << task->getTask();
    }
    qDebug() << taskList;
    overwritingFile("tasks", taskList);
    m_socket->syncTasks({task});
}
void MainWindow::onTaskEdited(TaskForm *task) {
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime deadline = task->getDeadlineDateTime();
    if(notifiedEndingSoon.contains(task) && currentTime < deadline && currentTime.secsTo(deadline) > 3600)
    {
        notifiedEndingSoon.remove(task);
    }
    else if(!notifiedEndingSoon.contains(task) && currentTime.secsTo(deadline) > 0 && currentTime.secsTo(deadline) <= 3600)
    {
        notifiedEndingSoon.insert(task);
    }
    if(notifiedMissedDeadline.contains(task) && currentTime < deadline)
    {
        notifiedMissedDeadline.remove(task);
    }
    else if(!notifiedMissedDeadline.contains(task) && currentTime >= deadline)
    {
        notifiedMissedDeadline.insert(task);
    }
    overwritingFile("tasks", taskList);
    m_socket->syncTasks({task});
}
void MainWindow::showAllTasks() {
    currentFilter = All;
    updateTaskVisibility();
    ui->allTasks->setStyleSheet("QPushButton { background-color: none; border: 3px solid rgb(133, 139, 225) ; font-size:40px; }");
    ui->completingTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->deadlineEnded->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completedTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
}

void MainWindow::showEndingSoonTasks() {
    currentFilter = EndingSoon;
    updateTaskVisibility();
    ui->completingTasks->setStyleSheet("QPushButton { background-color: none; border: 3px solid rgb(133, 139, 225) ; font-size:40px; }");
    ui->allTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->deadlineEnded->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completedTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
}

void MainWindow::showMissedDeadlineTasks() {
    currentFilter = MissedDeadline;
    updateTaskVisibility();
    ui->deadlineEnded->setStyleSheet("QPushButton { background-color: none; border: 3px solid rgb(133, 139, 225) ; font-size:40px; }");
    ui->allTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completingTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completedTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
}

void MainWindow::showCompletedTasks() {
    currentFilter = Completed;
    updateTaskVisibility();
    ui->completedTasks->setStyleSheet("QPushButton { background-color: none; border: 3px solid rgb(133, 139, 225) ; font-size:40px; }");
    ui->allTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->completingTasks->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
    ui->deadlineEnded->setStyleSheet("QPushButton { background-color: none; border: none ; font-size:40px; } QPushButton:hover { border-bottom: 3px solid rgb(216, 236, 255) ;}");
}
void MainWindow::updateTaskVisibility()
{
    ui->scrollAreaWidgetContents->setUpdatesEnabled(false);
    QDateTime currentTime = QDateTime::currentDateTime();
    std::sort(taskList->begin(), taskList->end(), [](TaskForm* a, TaskForm* b) {
        if (a->isDone() != b->isDone()) {
            return !a->isDone();
        }
        return a->getDeadlineDateTime() < b->getDeadlineDateTime();
    });
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        QWidget *widget = child->widget();
        if (widget) {
            widget->setParent(nullptr);
        }
        delete child;
    }

    for (TaskForm *task : *taskList) {
        bool shouldShow = false;
        QDateTime deadline = task->getDeadlineDateTime();

        switch (currentFilter) {
        case All:
            shouldShow = true;
            break;

        case EndingSoon:
            shouldShow = currentTime.secsTo(deadline) > 0 &&
                         currentTime.secsTo(deadline) <= 3600 && !task->isDone();
            break;

        case MissedDeadline:
            shouldShow = currentTime >= deadline && !task->isDone();
            break;

        case Completed:
            shouldShow = task->isDone();
            break;
        }
        if(!task->isDone())
        {
            if (currentTime.secsTo(deadline) > 0 && currentTime.secsTo(deadline) <= 3600)
            {
                if (!notifiedEndingSoon.contains(task)) {
                    if(config->enableTextNotifications)
                    {
                        trayIcon->showMessage(tr("Notification"), tr("The deadline for completing the task is coming to an end!\nThe task: ") + task->getTask() + tr("\nDeadline: ") + task->getDeadline(), QSystemTrayIcon::Information);
                    }
                    if(config->enableSoundNotifications)
                    {
                        soundEffect->setSource(QUrl("qrc:/sounds/sounds/miui6-notification-meloboom.wav"));
                        soundEffect->setVolume(1.0);
                        soundEffect->play();
                    }
                    notifiedEndingSoon.insert(task);
                }
            }
            if (currentTime >= deadline)
            {
                if (!notifiedMissedDeadline.contains(task)) {
                    if(config->enableTextNotifications)
                    {
                        trayIcon->showMessage(tr("Notification"), tr("The task missed the deadline!\nThe task: ") + task->getTask() + tr("\nDeadline: ") + task->getDeadline(), QSystemTrayIcon::Information);
                    }
                    if(config->enableSoundNotifications)
                    {
                        soundEffect->setSource(QUrl("qrc:/sounds/sounds/logoff-meloboom.wav"));
                        soundEffect->setVolume(1.0);
                        soundEffect->play();
                    }
                    notifiedMissedDeadline.insert(task);
                }
            }
        }
        if(task->isDone())
        {
            task->setStyleSheetForWidget("QWidget { background-color: white; border: 2px solid gray; } QLabel { border: none; border-right: 2px solid gray; color: gray;} QCheckBox { border: none; border-right: 2px solid gray;} QPushButton { border: none;}");
        }
        else if(currentTime >= deadline)
        {
            task->setStyleSheetForWidget("QWidget { background-color: white; border: 2px solid red; } QLabel { border: none; border-right: 2px solid red; color: red;} QCheckBox { border: none; border-right: 2px solid red;} QPushButton { border: none;}");
        }
        else if(currentTime.secsTo(deadline) > 0 && currentTime.secsTo(deadline) <= 3600)
        {
            task->setStyleSheetForWidget("QWidget { background-color: white; border: 2px solid orange; } QLabel { border: none; border-right: 2px solid orange; color: orange;} QCheckBox { border: none; border-right: 2px solid orange;} QPushButton { border: none;}");
        }
        else
        {
            task->setStyleSheetForWidget("QWidget { background-color: white; border: 2px solid black; } QLabel { border: none; border-right: 2px solid black; color: black;} QCheckBox { border: none; border-right: 2px solid black;} QPushButton { border: none;}");
        }
        if (shouldShow) {
            layout->addWidget(task);
        }
        task->setVisible(shouldShow);
    }
    ui->scrollAreaWidgetContents->setUpdatesEnabled(true);
}
void MainWindow::showButtonWarning()
{
    ui->warningButton->show();
}
void MainWindow::hideButtonWarning()
{
    if(m_socket->getStatusAuthorization())
        ui->warningButton->hide();
}

void MainWindow::onSettingsButtonClicked()
{
    Settings settings(config, this);

    settings.exec();

    ui->retranslateUi(this);

    openAction->setText(tr("Open"));
    quitAction->setText(tr("Exit"));

    loginUserMenu->setText(tr("Log in"));
    registrationUserMenu->setText(tr("Register"));

    changePasswordUserMenu->setText(tr("Change password"));
    exitUserMenu->setText(tr("Log out"));
}

void MainWindow::quitApplication() {
    trayIcon->hide();
    QApplication::quit();
}
