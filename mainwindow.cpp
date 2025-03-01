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
#include "edittask.h"
#include "settings.h"
#include "binarydatahandler.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , trayIcon(new QSystemTrayIcon(this))
    , trayMenu(new QMenu(this))
    , updateTimer(new QTimer(this))
    , soundEffect(new QSoundEffect(this))
    , taskList(new QList<TaskForm*>)
    , config(new Config())
{
    ui->setupUi(this);
    loadFromFile("config", config);
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
            connect(task, &TaskForm::changeDoneTask, this, &MainWindow::changeDoneTask);
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

    QAction *openAction = new QAction("Открыть", this);
    QAction *quitAction = new QAction("Выйти", this);

    connect(openAction, &QAction::triggered, this, &MainWindow::showWindow);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApplication);

    trayMenu->addAction(openAction);
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

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
        "Приложение свернуто",
        "Приложение было свернуто в трей. Нажмите на иконку, чтобы открыть.",
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
void MainWindow::onAddButtonClicked()
{
    EditTask addTask(this);
    addTask.setWindowTitle("Add task");
    addTask.setButtonAcceptText("ДОБАВИТЬ");
    TaskForm *newTask = new TaskForm(ui->scrollAreaWidgetContents);
    QDateTime currentTime = QDateTime::currentDateTime();
    //QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());

    switch (addTask.exec()) {
    case QDialog::Accepted:
        qDebug() << "Accepted";
        newTask->setParameters(addTask.getTask(), addTask.getDeadlineTime(), addTask.getDeadlineDateTime());
        //layout->addWidget(newTask);
        taskList->append(newTask);
        connect(newTask, &TaskForm::taskDeleted, this, &MainWindow::onTaskDeleted);
        connect(newTask, &TaskForm::taskEdited, this, &MainWindow::onTaskEdited);
        connect(newTask, &TaskForm::changeDoneTask, this, &MainWindow::changeDoneTask);
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

void MainWindow::onTaskDeleted(TaskForm *task) {
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
}
void MainWindow::onTaskEdited(TaskForm *task) {
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime deadline = task->getDeadlineDateTime();
    if(notifiedEndingSoon.contains(task) && currentTime < deadline && currentTime.secsTo(deadline) > 3600)
    {
        notifiedEndingSoon.remove(task);
    }
    if(notifiedMissedDeadline.contains(task) && currentTime < deadline)
    {
        notifiedMissedDeadline.remove(task);
    }
    overwritingFile("tasks", taskList);
}
void MainWindow::changeDoneTask()
{
    overwritingFile("tasks", taskList);
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
                        trayIcon->showMessage("Уведомление", "Дедлайн выполнения задачи подходит к концу \nЗадача: " + task->getTask() + "\nДедлайн: " + task->getDeadline(), QSystemTrayIcon::Information);
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
                        trayIcon->showMessage("Уведомление", "Задача пропустила дедлайн!\n Задача: " + task->getTask() + "\n Дедлайн: " + task->getDeadline(), QSystemTrayIcon::Information);
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
        qDebug() << currentTime << "currentTime" << deadline << "deadline";
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

void MainWindow::onSettingsButtonClicked()
{
    Settings settings(config, this);

    settings.exec();
}

void MainWindow::quitApplication() {
    trayIcon->hide();
    QApplication::quit();
}
