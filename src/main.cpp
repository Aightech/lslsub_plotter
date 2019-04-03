/**
 * \file main.cpp
 * \brief TODO.
 * \author Alexis Devillard
 * \version 1.0
 * \date 03 march 2019
 */

#include "include/mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
