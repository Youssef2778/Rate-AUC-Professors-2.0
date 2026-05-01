cmake_minimum_required(VERSION 3.5)

project(Rate_AUC VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)
find_package(Boost REQUIRED COMPONENTS json)
find_package(GTest REQUIRED)

find_package(OpenSSL REQUIRED)

set(PROJECT_SOURCES
        ./src/main.cpp
        ./src/mainwindow.cpp
        ./Include/mainwindow.h
        ./src/mainwindow.ui
        ./src/loginPage.cpp
        ./src/registerPage.cpp
        ./src/professorPage.cpp
        ./src/leaderboardPage.cpp
        ./src/homePage.cpp
        ./src/helpers.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Rate_AUC
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
    )
else()
    if(ANDROID)
        add_library(Rate_AUC SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(Rate_AUC
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Rate_AUC PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(Rate_AUC PRIVATE Boost::json)
target_link_libraries(Rate_AUC PRIVATE bcrypt)
target_include_directories(Rate_AUC PRIVATE Include)

add_executable(server server.cpp server.hpp)
target_link_libraries(server PRIVATE Boost::json)
target_link_libraries(server PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(server PRIVATE mysqlcppconn)
target_link_libraries(server PRIVATE bcrypt)
target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)



if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Rate_AUC)
endif()
set_target_properties(Rate_AUC PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Rate_AUC
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Rate_AUC)
endif()

enable_testing()

add_executable(RateAUC_test ./tests/tests.cpp ./src/helpers.cpp)
target_link_libraries(RateAUC_test PRIVATE GTest::gtest_main GTest::gmock)
target_link_libraries(RateAUC_test PRIVATE Boost::json)

target_include_directories(RateAUC_test PRIVATE Include)

add_executable(RateAUC_QTtest ./tests/qt_tests.cpp ${PROJECT_SOURCES})
target_link_libraries(RateAUC_QTtest PRIVATE GTest::gtest_main Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network GTest::gmock)
target_link_libraries(RateAUC_QTtest PRIVATE Boost::json)
target_link_libraries(RateAUC_QTtest PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(RateAUC_QTtest PRIVATE mysqlcppconn)
target_link_libraries(RateAUC_QTtest PRIVATE bcrypt)
target_link_libraries(RateAUC_QTtest PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(RateAUC_QTtest PRIVATE Include)


include(GoogleTest)
gtest_discover_tests(RateAUC_QTtest DISCOVERY_MODE PRE_TEST)
