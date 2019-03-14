#include "include/mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox_stream->addItem("LSL Stream", 0);
    ui->comboBox_stream->addItem("IP Stream", 1);
    ui->comboBox_stream->addItem("Serial Stream", 2);
    //setup the objects to connect the lsl layer (button + line edit)
    ui->lineEdi_stream->setText("EEG");
    connect(ui->pushButton_connect_stream, SIGNAL (released()), this, SLOT (connect_stream()));
    connect(ui->comboBox_stream, SIGNAL (currentIndexChanged(int)), this, SLOT(change_stream(int)));

    ui->checkBox_3D->setChecked(true);

    //init the timer object used to update the graph
    QObject::connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(100);
    m_timer.start();





    for(uint i =0; i < 2; i++)
    {
        m_nb_channels.push_back(8);
        m_time_spans.push_back(30);
        std::vector<std::vector<double>> data;
        std::vector<double> v(m_nb_channels[i]);
        m_data.push_back(data);
        for(uint t =0; t < m_time_spans[i]; t++)
            m_data[i].push_back(v);
    }

    init3DGraph();

    init2DGraph();

    // Set the colors from the light theme as default ones
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
    pal.setColor(QPalette::WindowText, QRgb(0x404044));
    qApp->setPalette(pal);



}

void MainWindow::init2DGraph()
{
    for(uint i =0; i < 2; i++)
    {
        m_chart2D.push_back(new QtCharts::QChart());
        //chart->setTitle("Line chart");

        //store the m_data to a graphic object
        for (uint n = 0 ; n < m_nb_channels[i]; n++) {
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[i]);
            for (unsigned int t = 0; t < m_data.size(); t++)
                series->append(QPointF(t,m_data[i][t][n]));
            series->setName(QString::number(n));
            m_chart2D[i]->addSeries(series);
        }

        //set up the chart
        m_chart2D[i]->createDefaultAxes();
        m_chart2D[i]->axes(Qt::Horizontal).first()->setRange(-1, m_time_spans[i]);
        m_chart2D[i]->axes(Qt::Vertical).first()->setRange(-10, 10);

        QtCharts::QChartView *chartView = new QtCharts::QChartView(m_chart2D[i]);
        chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        chartView->chart()->legend()->hide();
        if(i==0)
            ui->horizontalLayout_2D->addWidget(chartView);
        else
            ui->horizontalLayout_2D_2->addWidget(chartView);


    }

}

void MainWindow::init3DGraph()
{
    for(uint i =0; i < 2; i++)
    {
        m_graph.push_back(new QtDataVisualization::Q3DSurface());
        QWidget *container = QWidget::createWindowContainer(m_graph[i]);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if(i==0)
            ui->horizontalLayout_3D->addWidget(container);
        else
            ui->horizontalLayout_3D_2->addWidget(container);

        //store the m_data to a graphic object
        m_proxy_chart3D.push_back(new QtDataVisualization::QSurfaceDataProxy());
        m_chart3D.push_back(new QtDataVisualization::QSurface3DSeries(m_proxy_chart3D[i]));
        QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
        dataArray->reserve(static_cast<int>(m_time_spans[i]));
        for (uint t = 0 ; t < m_time_spans[i] ; t++) {
            QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(static_cast<int>(m_nb_channels[i]));
            for (uint n = 0; n < m_nb_channels[i]; n++) {
                (*newRow)[static_cast<int>(n)].setPosition(QVector3D(t, static_cast<float>(m_data[i][t][n]), n));
            }
            *dataArray << newRow;
        }
        m_proxy_chart3D[i]->resetArray(dataArray);

        //set up the graph and the chart
        m_chart3D[i]->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurfaceAndWireframe);
        m_chart3D[i]->setFlatShadingEnabled(false);
        m_chart3D[i]->setItemLabelVisible(false);
        //m_chart3D[i]->
        m_graph[i]->addSeries(m_chart3D[i]);
        m_graph[i]->axisX()->setRange(-1, m_time_spans[i]);
        m_graph[i]->axisY()->setRange(-5, 5);
        m_graph[i]->axisZ()->setRange(-1, m_nb_channels[i]);
        m_graph[i]->axisX()->setLabelAutoRotation(30);
        m_graph[i]->axisY()->setLabelAutoRotation(90);
        m_graph[i]->axisZ()->setLabelAutoRotation(30);
        m_graph[i]->setReflection(false);
        m_graph[i]->setReflectivity(0);
        m_graph[i]->setOrthoProjection(true);


        //great a gradient to see the heat map
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::darkGreen);
        gr.setColorAt(0.5, Qt::yellow);
        gr.setColorAt(0.8, Qt::red);
        gr.setColorAt(1.0, Qt::darkRed);
        m_graph[i]->seriesList().at(0)->setBaseGradient(gr);
        m_graph[i]->seriesList().at(0)->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);

        m_graph[i]->scene()->activeCamera()->setCameraPosition(0, 90);
        m_graph[i]->scene()->activeCamera()->setZoomLevel(400);


    }


}

void MainWindow::connect_stream()
{
    ui->pushButton_connect_stream->setDisabled(true);
    ui->lineEdi_stream->setDisabled(true);
    ui->comboBox_stream->setDisabled(true);

    if(ui->comboBox_stream->currentIndex()==0)
    {//LSL stream
        std::string stream_label = ui->lineEdi_stream->text().toStdString();
        std::cout << "Trying to connect to \"" << stream_label << "\""<< std::endl;
        std::vector<lsl::stream_info> results = lsl::resolve_stream("name",stream_label);
        m_inlets.push_back( new lsl::stream_inlet(results[0]));
    }
    if(ui->comboBox_stream->currentIndex()==1)
    {// TODO IP stream
        //std::string host = ui->lineEdi_stream->text().toStdString();
        // m_otb_client.connect(host);
        // m_otb_client.start();
    }
    if(ui->comboBox_stream->currentIndex()==2)
    {// TODO Serial stream
    }


}

void MainWindow::handleTimeout()
{
    //if an lsl stream is connected
    for(uint i =0; i< m_inlets.size(); i++)
    {
        std::vector<float> sample;
        m_inlets[i]->pull_sample(sample);//get the sample
        m_nb_channels[i] = static_cast<uint>(sample.size());
        //store it in our data array
        for(unsigned int n = 0 ; n < sample.size(); n++)
            m_data[i][(m_t)%m_time_spans[i]][n] = static_cast<double>(sample[n]);

        m_t++;
    }

    //update the 3D graph
    update3Dgraph();

    /*update the 2D graph*/
    update2Dgraph();



}

void MainWindow::update3Dgraph()
{
    for(uint i =0; i <2 ;i++)
    {
        if((i==0)?ui->checkBox_3D->isChecked():ui->checkBox_3D_2->isChecked())
        {
            //allocate a new surface (in x axis)
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            dataArray->reserve(static_cast<int>(m_time_spans[i]));
            for (uint t = 0 ; t < m_time_spans[i] ; t++) {
                //allocate a new surface (in Z axis
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(static_cast<int>(m_nb_channels[i]));
                for (uint n = 0; n < m_nb_channels[i]; n++) {
                    //draw a point at the coresponding point
                    //the data  array is use as a roundrobin
                    (*newRow)[static_cast<int>(n)].setPosition(QVector3D(t+m_t, static_cast<float>(m_data[i][((m_t)%m_time_spans[i]+t+1)%m_time_spans[i]][n]), n));
                }
                *dataArray << newRow;
            }

            //update the time window of the graph
            m_graph[i]->axisX()->setRange(m_t-2, m_time_spans[i] + m_t+1);
            m_proxy_chart3D[i]->resetArray(dataArray);
        }
    }
}


void MainWindow::update2Dgraph()
{
    for(uint i =0; i<2; i++)
    {
        if( ((i==0)?ui->checkBox_2D->isChecked():ui->checkBox_2D_2->isChecked()) )
        {
            m_chart2D[i]->removeAllSeries();
            for (uint n = 0 ; n < m_nb_channels[i]; n++) {
                QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[i]);
                for (unsigned int t = 0; t < m_data.size(); t++)
                    series->append(QPointF(t+m_t, m_data[i][((m_t)%m_time_spans[i]+t+1)%m_time_spans[i]][n]));
                series->setName(QString::number(n));
                m_chart2D[i]->addSeries(series);
            }
            m_chart2D[i]->axes(Qt::Horizontal).first()->setRange(m_t-2, m_time_spans[i] + m_t+1);
        }
    }
}

void MainWindow::change_stream(int index)
{
    if(index == 0)
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("LSL Stream Label");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("");
        ui->lineEdit_stream_port->setPlaceholderText("None");
    }
    else if(index == 1 )
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("IP address hosting OTB software. Ex: localhost");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("31000");
    }
    else if(index == 2)
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("Serial Port Name");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("");
        ui->lineEdit_stream_port->setPlaceholderText("None");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
