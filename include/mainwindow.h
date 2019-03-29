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


    void update3Dgraph(unsigned);
    void update2Dgraph(unsigned);

    void initDataArray(unsigned);
    void createDataArray(unsigned);
    void create3Dgraph(unsigned index, std::string name);
    void create2Dgraph(unsigned index, std::string name);

    bool isStreamsTicked(std::string name);
    int StreamsIndex(QString name);
    void storeChunk_int16(unsigned index);
    void storeChunk_float(unsigned index);


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

    std::vector<unsigned int> m_counter;
    std::vector<double> m_Xmin;
    std::vector<double> m_Xmax;
    std::vector<unsigned> m_XnbSample;//time span

    std::vector<double> m_Ymin;
    std::vector<double> m_Ymax;
    std::vector<double> m_YminDisp;
    std::vector<double> m_YmaxDisp;

    std::vector<double> m_Zmin;
    std::vector<double> m_Zmax;
    std::vector<unsigned> m_ZnbSample;//nb channels

    std::vector<unsigned long> m_chunk_size;
    std::vector<unsigned> m_mean_span;

    std::vector<unsigned> m_stream3Dstate;
    std::vector<unsigned> m_stream2Dstate;

    std::vector<std::vector<bool>> m_channelIsValid;


    std::vector<std::vector<std::vector<double>>> m_data;

    std::vector<QtDataVisualization::Q3DSurface*> m_graph;
    std::vector<QtCharts::QChart *> m_chart2D;
    std::vector<QtDataVisualization::QSurface3DSeries*> m_chart3D;
    std::vector<QtDataVisualization::QSurfaceDataProxy*> m_proxy_chart3D;


};



#endif // MAINWINDOW_H
