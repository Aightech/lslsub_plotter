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

#pragma pack(1)
typedef struct _data_sample {
        short channel[408];
} data_sample;


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

    void update3Dgraph(unsigned index);
    void update2Dgraph();

    Ui::MainWindow *ui;
    QTimer m_timer;
    std::vector<lsl::stream_inlet *> m_inlets;

    unsigned int m_t[2];
    std::vector<unsigned int> m_time_spans;
    std::vector<std::vector<double>> m_time_stamps;
    std::vector<long> m_min_ranges;
    std::vector<long> m_max_ranges;
    std::vector<double> m_last_time_stamps;
    double m_current_time_stamp;
    double m_past_time_stamp;
    std::vector<unsigned> m_chunk_size;

    std::vector<unsigned int> m_nb_channels;

    std::vector<std::vector<std::vector<double>>> m_data;
    std::vector<std::vector<short>> m_samples_short;

    std::vector<QtDataVisualization::Q3DSurface*> m_graph;
    std::vector<QtCharts::QChart *> m_chart2D;
    std::vector<QtDataVisualization::QSurface3DSeries*> m_chart3D;
    std::vector<QtDataVisualization::QSurfaceDataProxy*> m_proxy_chart3D;


};

#endif // MAINWINDOW_H
