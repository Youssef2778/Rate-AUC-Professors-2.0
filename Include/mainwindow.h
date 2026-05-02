#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "User.h"
#include "Comment.h"
#include <QMainWindow>
#include <QPixmap>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <unordered_map>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <vector>

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
    User *user;

   private:
    // Map department_name to id
    Ui::MainWindow *ui;
    std::unordered_map<std::string, int> Deps;
    std::unordered_map<int, std::string> Courses;
    std::unordered_map<int, std::string> Profs;
	std::vector<Comment> Comments;

    std::vector<int> selected_flairs;



    // ... your other private variables
    QNetworkAccessManager *networkManager;
    void fetchAiSummary(std::string courseId, std::string profId);
    // We'll use a unique_ptr so we can reconnect if needed
    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;

    // A function we can call anytime to refresh the list
    void refreshList();
    bool Connected;

    // For persistent connection to be established once at startup
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket{io};

    void EstablishConnection();
    void Reconnect();
    void CenterWidget(int pageIndex, QWidget *TargetWidget);
    void LoginPage();
    void RegisterPage();
    void HomePage();
    void LeaderboardPage();
    void professorPage();
    void Logout();
	void DisplayComments();
	void CreateComment(Comment comment);
    void ClearLayout(QLayout* layout);
    virtual bool presenceChecks(QString username, QString password, QString passwordTwo, QString email);




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
    // ✅ NEW: Added declarations for the upvote and downvote handlers
    void handleUpvote(const std::string& profID, int courseID, std::string CourseName);
    void handleDownvote(const std::string& profID, int courseID, std::string CourseName);
    void on_postComment_clicked();
    void on_mehFlair_clicked(bool checked);
    void on_avoidFlair_clicked(bool checked);
    void on_recommendFlair_clicked(bool checked);
    void on_easyAFlair_clicked(bool checked);
    void on_highWorkLoadFlair_clicked(bool checked);
    void on_fastPacedFlair_clicked(bool checked);
    void on_BackToHomepage_clicked();
    void on_BackToLeaderboard_clicked();
    void on_logoutButton_clicked();
    void on_logoutButton_2_clicked();
    void on_logoutButton_3_clicked();
};

#endif  // MAINWINDOW_H
