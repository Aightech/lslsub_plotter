#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/**
 * \file mainwindow.h
 * \brief TODO.
 * \author Alexis Devillard
 * \version 1.0
 * \date 03 march 2019
 */

#include <QtDataVisualization>
#include <QSurfaceDataProxy>
/*#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QHeightMapSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>*/

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <Q3DSurface>
#include <QMainWindow>
#include <QtCore/QTimer>
#include <qtreewidget.h>

#include <lsl_cpp.h>
#include <vector>
#include <iostream>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void scanStream();
    void plot_line();
    void plot_heatmap();

private:

    Ui::MainWindow *ui;
    QTimer m_timer;
    unsigned m_stream_count=0;
    int m_selectedStream=-1;
    std::vector<lsl::stream_inlet*> m_inlet;
    std::vector<lsl::stream_info> m_results;
    std::string channel_format_str[9] { "none",
            "cf_float32",
            "cf_double64",
            "cf_string",
            "cf_int32",
            "cf_int16",
            "cf_int8",
            "cf_int64",
            "cf_undefined"     // Can not be transmitted.
           };



};



#endif // MAINWINDOW_H
