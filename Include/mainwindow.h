#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "User.h"
#include <QMainWindow>
#include <QPixmap>
#include <QResizeEvent>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *ui;

   private:
    // Map department_name to id
    std::unordered_map<std::string, int> Deps;
    std::unordered_map<std::string, int> Courses;
    std::unordered_map<std::string, std::string> Profs;
	std::vector<Comment> Comments;


    User *user;

    // For persistent connection to be established once at startup
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket{io};

    void EstablishConnection();
    void CenterWidget(int pageIndex, QWidget *TargetWidget);
    void LoginPage();
    void RegisterPage();
    void HomePage();
    void LeaderboardPage(int);
    void Logout();
    void professorPage();


   private slots:
    void on_checkBox_4_stateChanged(int arg1);

    //     void on_register_label_linkActivated(const QString &link);

    void on_register_label_4_linkActivated(const QString &link);
    void on_checkBox_6_stateChanged(int arg1);
    void on_register_label_6_linkActivated(const QString &link);
    void on_pushButton_6_clicked();
    void on_pushButton_4_clicked();
    void on_DepartmentCB_currentIndexChanged(int index);
    void on_pushButton_clicked();
};

#endif  // MAINWINDOW_H
