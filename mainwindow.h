#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QTimer>
#include <QList>
#include <QSoundEffect>
#include "taskform.h"
#include "config.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum FilterType {
    All,
    EndingSoon,
    MissedDeadline,
    Completed
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void showWindow();
    void quitApplication();
    void onAddButtonClicked();
    void onSettingsButtonClicked();

    void onTaskDeleted(TaskForm *task);
    void onTaskEdited(TaskForm *task);
    void showAllTasks();
    void showEndingSoonTasks();
    void showMissedDeadlineTasks();
    void showCompletedTasks();
    void updateTaskVisibility();

    void changeDoneTask();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    FilterType currentFilter = FilterType::All;
    QList<TaskForm*>* taskList;
    QTimer *updateTimer;

    QSet<TaskForm*> notifiedEndingSoon;
    QSet<TaskForm*> notifiedMissedDeadline;

    QSoundEffect* soundEffect;

private:
    Config *config;

};
#endif // MAINWINDOW_H
