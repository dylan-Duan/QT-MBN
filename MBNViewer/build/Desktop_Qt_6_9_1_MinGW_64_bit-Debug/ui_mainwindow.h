/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCharts/QChartView>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *btnBrowse;
    QPushButton *btnLoad;
    QPushButton *btnPlotTime;
    QPushButton *btnPlotFreq;
    QLineEdit *editDir;
    QChartView *widget;
    QTextEdit *textLog;
    QPushButton *btnPlotEnv;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        btnBrowse = new QPushButton(centralwidget);
        btnBrowse->setObjectName("btnBrowse");
        btnBrowse->setGeometry(QRect(20, 10, 121, 31));
        btnLoad = new QPushButton(centralwidget);
        btnLoad->setObjectName("btnLoad");
        btnLoad->setGeometry(QRect(20, 50, 161, 31));
        btnPlotTime = new QPushButton(centralwidget);
        btnPlotTime->setObjectName("btnPlotTime");
        btnPlotTime->setGeometry(QRect(390, 50, 151, 31));
        btnPlotFreq = new QPushButton(centralwidget);
        btnPlotFreq->setObjectName("btnPlotFreq");
        btnPlotFreq->setGeometry(QRect(210, 50, 151, 31));
        editDir = new QLineEdit(centralwidget);
        editDir->setObjectName("editDir");
        editDir->setGeometry(QRect(180, 10, 231, 31));
        widget = new QChartView(centralwidget);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(20, 80, 751, 341));
        textLog = new QTextEdit(centralwidget);
        textLog->setObjectName("textLog");
        textLog->setGeometry(QRect(20, 430, 751, 131));
        btnPlotEnv = new QPushButton(centralwidget);
        btnPlotEnv->setObjectName("btnPlotEnv");
        btnPlotEnv->setGeometry(QRect(570, 50, 171, 31));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 18));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnBrowse->setText(QCoreApplication::translate("MainWindow", "Browse", nullptr));
        btnLoad->setText(QCoreApplication::translate("MainWindow", "Load", nullptr));
        btnPlotTime->setText(QCoreApplication::translate("MainWindow", "Time Domain", nullptr));
        btnPlotFreq->setText(QCoreApplication::translate("MainWindow", "Frequency Domain", nullptr));
        btnPlotEnv->setText(QCoreApplication::translate("MainWindow", "Envelope", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
