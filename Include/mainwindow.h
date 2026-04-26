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

   private:
    // Map department_name to id
    Ui::MainWindow *ui;
    std::unordered_map<std::string, int> Deps;
    std::unordered_map<std::string, int> Courses;
    std::unordered_map<std::string, int> Profs;
	std::vector<Comment> Comments;

    int m_currentCourseId; // To remember which course we are viewing


    // ... your other private variables
    QNetworkAccessManager *networkManager;
    void fetchAiSummary(std::string courseId, std::string profId);
    // We'll use a unique_ptr so we can reconnect if needed
    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;

    // A function we can call anytime to refresh the list
    void refreshList(std::string);
    User *user;

    // For persistent connection to be established once at startup
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket{io};

    void EstablishConnection();
    void CenterWidget(int pageIndex, QWidget *TargetWidget);
    void LoginPage();
    void RegisterPage();
    void HomePage();
    void LeaderboardPage(std::string);
    void professorPage(std::string, std::string);
    void Logout();
	void DisplayComments();
	void CreateComment(Comment comment);
    void ClearLayout(QLayout* layout);




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
    void on_backButton_clicked();
    void on_postComment_clicked();
};

#endif  // MAINWINDOW_H
