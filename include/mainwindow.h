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
#include <qtreewidget.h>

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
    void changeChannelsRange();
    void changeHeatMapRange();
    void scanStream();
    void changeXnbSample();
    void changeMeanSpan();
    void updateGUI();

private:
    void handleTimeout();
    void init3DGraph();


    void update3Dgraph(int);
    void update2Dgraph(int);

    void initDataArray(int);
    void createDataArray(int);
    void create3Dgraph(int index, std::string name);
    void create2Dgraph(int index, std::string name);

    int selectedStream();


    Ui::MainWindow *ui;
    QTimer m_timer;
    int m_stream_count=0;
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

    std::vector<unsigned int> m_counter;
    std::vector<float> m_Xmin;
    std::vector<float> m_Xmax;
    std::vector<int> m_XnbSample;//time span

    std::vector<float> m_Ymin;
    std::vector<float> m_Ymax;

    std::vector<float> m_Zmin;
    std::vector<float> m_Zmax;
    std::vector<int> m_ZnbSample;//nb channels

    std::vector<unsigned> m_chunk_size;
    std::vector<unsigned> m_mean_span;

    std::vector<unsigned> m_stream3Dstate;
    std::vector<unsigned> m_stream2Dstate;


    std::vector<std::vector<std::vector<float>>> m_data;

    std::vector<QtDataVisualization::Q3DSurface*> m_graph;
    std::vector<QtCharts::QChart *> m_chart2D;
    std::vector<QtDataVisualization::QSurface3DSeries*> m_chart3D;
    std::vector<QtDataVisualization::QSurfaceDataProxy*> m_proxy_chart3D;


};



#endif // MAINWINDOW_H
