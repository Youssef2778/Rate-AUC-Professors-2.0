#include <QtTest/QtTest>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QComboBox>
#include "../Include/mainwindow.h"

class TestMainWindow : public QObject {
    Q_OBJECT

private:
    MainWindow *window;

private slots:
    // Runs before EACH test
    void init() {
        window = new MainWindow();
        window->show();  // important for UI interaction
    }

    // Runs after EACH test
    void cleanup() {
        delete window;
    }

    void testLoginPageError() {
        QLineEdit* email = window->findChild<QLineEdit*>("email_login_lineEdit");
        QLineEdit* password = window->findChild<QLineEdit*>("password_login_lineEdit");
        QPushButton* loginButton = window->findChild<QPushButton*>("pushButton_4");
        QTest::keyClicks(email, "Hello");
        QTest::keyClicks(password, "123");

        QSignalSpy spy(loginButton, &QPushButton::clicked);

        QTest::mouseClick(loginButton, Qt::LeftButton);

        // Ensures that a signal is emitted
        QCOMPARE(spy.count(), 1);

        QLabel* email_not_registered_error = window->findChild<QLabel*>("email_notFound_error");

        QVERIFY(email_not_registered_error->isVisible());
    }

    void testRegisterPageError() {
        QLabel* registerLabel = window->findChild<QLabel*>("register_label_4");
        QStackedWidget* stackedWidget = window->findChild<QStackedWidget*>("stackedWidget");
        QSignalSpy spy(registerLabel, &QLabel::linkActivated);
        QTest::mouseClick(registerLabel, Qt::LeftButton);
        emit registerLabel->linkActivated(" ");  
        QCOMPARE(spy.count(), 1);

        // Checks whether we landed on the register page or not
        QCOMPARE(stackedWidget->currentIndex(), 1);

        // Let's click on the register button
        QPushButton* registerButton = window->findChild<QPushButton*>("pushButton_6");
        QTest::mouseClick(registerButton, Qt::LeftButton);

        // Ensures that the proper error message pops up.
        QLabel* username_notFound_error = window->findChild<QLabel*>("empty_username_error");
        QVERIFY(username_notFound_error->isVisible());
    }

    void testFetchingOfDepartmentsFromServer() {
        QPushButton* loginButton = window->findChild<QPushButton*>("pushButton_4");
        QStackedWidget* stackedWidget = window->findChild<QStackedWidget*>("stackedWidget");
        QSignalSpy spy(loginButton, &QPushButton::clicked);
        QTest::mouseClick(loginButton, Qt::LeftButton);

        // Ensures that a signal is emitted
        QCOMPARE(spy.count(), 1);

        // Check whether we are at the home page or not
        QCOMPARE(stackedWidget->currentIndex(), 3);

        QComboBox* departmentCB = window->findChild<QComboBox*>("DepartmentCB");

        // We are checking whether we successfully fetched the departments from the server or not.
        QVERIFY(departmentCB->count() > 0);
    }

    void testLogout() {
        QPushButton* loginButton = window->findChild<QPushButton*>("pushButton_4");
        QStackedWidget* stackedWidget = window->findChild<QStackedWidget*>("stackedWidget");
        QSignalSpy spy(loginButton, &QPushButton::clicked);
        QTest::mouseClick(loginButton, Qt::LeftButton);

        // Ensures that a signal is emitted
        QCOMPARE(spy.count(), 1);

        // Check whether we are at the home page or not
        QCOMPARE(stackedWidget->currentIndex(), 3);

        QWidget* profileCard = window->findChild<QWidget*>("profileCard");

        // This is where the user, their email, and the logout button exists.
        QVERIFY(profileCard->isVisible());

        QPushButton* logOut = window->findChild<QPushButton*>("logoutButton");
        QVERIFY(logOut->isVisible());
        QSignalSpy spy2(logOut, &QPushButton::clicked);
        // Logging out....
        QTest::mouseClick(logOut, Qt::LeftButton);

   
        QCOMPARE(spy2.count(), 1);

        // Making sure that we are on the login page.
        QCOMPARE(stackedWidget->currentIndex(), 0);

        // Make sure that our session "cookie" is reset.
        QCOMPARE(window->user, nullptr);
    }






    // 🧪 Test 1: Ensure QLineEdit exists
    // void testLineEditExists() {
    //     QLineEdit *lineEdit = window->findChild<QLineEdit*>("lineEdit");
    //     QVERIFY2(lineEdit != nullptr, "QLineEdit not found!");
    // }

    // 🧪 Test 2: Simulate typing
    // void testTyping() {
    //     QLineEdit *lineEdit = window->findChild<QLineEdit*>("lineEdit");
    //     QVERIFY(lineEdit != nullptr);

    //     lineEdit->clear();
    //     QTest::keyClicks(lineEdit, "Hello Qt");

    //     QCOMPARE(lineEdit->text(), QString("Hello Qt"));
    // }

    // 🧪 Test 3: Programmatic text change
    // void testSetText() {
    //     QLineEdit *lineEdit = window->findChild<QLineEdit*>("lineEdit");
    //     QVERIFY(lineEdit != nullptr);

    //     lineEdit->setText("Direct set");
    //     QCOMPARE(lineEdit->text(), QString("Direct set"));
    // }

    // 🧪 Test 4: Clear behavior
    // void testClear() {
    //     QLineEdit *lineEdit = window->findChild<QLineEdit*>("lineEdit");
    //     QVERIFY(lineEdit != nullptr);

    //     lineEdit->setText("Something");
    //     lineEdit->clear();

    //     QVERIFY(lineEdit->text().isEmpty());
    // }
};

// 🚀 Entry point generated automatically
QTEST_MAIN(TestMainWindow)
#include "qt_tests.moc"