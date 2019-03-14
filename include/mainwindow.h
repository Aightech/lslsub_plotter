#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtDataVisualization>
#include <QSurfaceDataProxy>
/*#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QHeightMapSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>*/

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <Q3DSurface>
#include <QMainWindow>
#include <QtCore/QTimer>

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
    void connect_stream();
    void change_stream(int);

private:
    void handleTimeout();
    void init3DGraph();
    void init2DGraph();

    void update3Dgraph();
    void update2Dgraph();

    Ui::MainWindow *ui;
    QTimer m_timer;
    std::vector<lsl::stream_inlet *> m_inlets;

    unsigned int m_t=100;
    std::vector<unsigned int> m_time_spans;
    std::vector<unsigned int> m_nb_channels;

    std::vector<std::vector<std::vector<double>>> m_data;
    std::vector<std::vector<short>> m_samples_short;

    std::vector<QtDataVisualization::Q3DSurface*> m_graph;
    std::vector<QtCharts::QChart *> m_chart2D;
    std::vector<QtDataVisualization::QSurface3DSeries*> m_chart3D;
    std::vector<QtDataVisualization::QSurfaceDataProxy*> m_proxy_chart3D;


};

#endif // MAINWINDOW_H
