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
    void changeChannelsRange();
    void changeHeatMapRange();
    void scanStream();

private:
    void handleTimeout();
    void init3DGraph();
    void init2DGraph();

    void update3Dgraph();
    void update2Dgraph();

    void createDataArray();


    Ui::MainWindow *ui;
    QTimer m_timer;
    lsl::stream_inlet* m_inlet=NULL;
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

    unsigned int m_counter=0;
    float m_Xmin=0;
    float m_Xmax=1000;
    int m_XnbSample=1000;//time span

    float m_Ymin=-100;
    float m_Ymax=100;

    float m_Zmin=0;
    float m_Zmax=408;
    int m_ZnbSample=408;//nb channels

    unsigned m_chunk_size;
    unsigned m_mean_span = 100;


    std::vector<std::vector<float>> m_data;

    QtDataVisualization::Q3DSurface* m_graph;
    QtCharts::QChart * m_chart2D;
    QtDataVisualization::QSurface3DSeries* m_chart3D;
    QtDataVisualization::QSurfaceDataProxy* m_proxy_chart3D;


};



#endif // MAINWINDOW_H
