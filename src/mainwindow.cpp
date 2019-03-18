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
    m_timer.setInterval(1);
    m_timer.start();





    for(uint i =0; i < 2; i++)
    {
        m_nb_channels.push_back(8);
        m_time_spans.push_back(10000);
        std::vector<std::vector<double>> data;
        std::vector<double> v(m_nb_channels[i]);
        m_data.push_back(data);
        std::vector<double> t;
        m_time_stamps.push_back(t);
        for(uint t =0; t < m_time_spans[i]; t++)
        {
            m_data[i].push_back(v);
            m_time_stamps[i].push_back(t*0.01);
        }
        m_chunk_size.push_back(0);
        m_last_time_stamps.push_back(0);
    }

    init3DGraph();

    //init2DGraph();

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
        for (uint t = 0 ; t < m_time_spans[i] ; t++)
        {
            QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(static_cast<int>(m_nb_channels[i]));
            for (uint n = 0; n < m_nb_channels[i]; n++)
            {
                float x = static_cast<float>(m_time_stamps[i][t]);
                float y = static_cast<float>(m_data[i][t][n]);
                float z = static_cast<float>(n);
                (*newRow)[static_cast<int>(n)].setPosition(QVector3D(x, y, z));
            }
            *dataArray << newRow;
        }
        m_proxy_chart3D[i]->resetArray(dataArray);

        //set up the graph and the chart
        m_chart3D[i]->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurface);
        m_chart3D[i]->setFlatShadingEnabled(false);
        m_chart3D[i]->setItemLabelVisible(false);
        //m_chart3D[i]->setMeshSmooth(false);
        m_graph[i]->addSeries(m_chart3D[i]);
        float min_t = static_cast<float>(m_time_stamps[i][0]);
        float max_t = static_cast<float>(m_time_stamps[i][m_time_spans[i]-1]);
        std::cout << min_t << " " << max_t << std::endl;
        m_graph[i]->axisX()->setRange(min_t, max_t);
        m_min_ranges.push_back(-5);
        m_max_ranges.push_back(5);
        m_graph[i]->axisY()->setRange(m_min_ranges[i], m_max_ranges[i]);
        m_graph[i]->axisZ()->setRange(-1, m_nb_channels[i]+1);


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
        m_graph[i]->setHorizontalAspectRatio(4.5);


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

        std::vector<std::vector<short>> chunk;
        std::vector<double> time_stamps;
        if(m_inlets[i]->pull_chunk(chunk,time_stamps))//get the sample
        {
            m_chunk_size[i] = static_cast<unsigned>(chunk.size());

            //if the number of channel has changed
            if(m_nb_channels[i] != chunk[0].size())
            {
                m_nb_channels[i] = static_cast<uint>(chunk[0].size());
                std::vector<std::vector<double>> data;
                std::vector<double> v(m_nb_channels[i]);
                m_data[i].clear();
                for(uint t =0; t < m_time_spans[i]; t++)
                    m_data[i].push_back(v);
                m_t[i]=0;
                m_graph[i]->axisX()->setRange(-1, m_time_spans[i]);
                m_graph[i]->axisZ()->setRange(-1, m_nb_channels[i]);
            }

            if(time_stamps.size()> 0 && time_stamps[0] + 0.0001 >= time_stamps[1])
            {
                double dt = 1/2048.0;//(time_stamps[0]- m_last_time_stamps[i])/static_cast<double>(time_stamps.size());
                std::cout << dt << std::endl;
                for(unsigned t = 0; t < time_stamps.size() ; t++)
                {
                    time_stamps[t] += t*dt;
                    //std::cout << time_stamps[t]<< std::endl;
                }
            }
            m_last_time_stamps[i]=time_stamps[0];
            //store it in our data array
            for(unsigned int t = 0 ; t < m_chunk_size[i] ; t++)
            {
                m_time_stamps[i][(m_t[i]+t)%m_time_spans[i]] = time_stamps[t];
                for (unsigned n = 0; n < chunk[t].size();n++)
                {
                    double val = chunk[t][n];
                    if(val < m_min_ranges[i])
                        m_min_ranges[i] = static_cast<long>(val);
                    if(val > m_max_ranges[i])
                        m_max_ranges[i] = static_cast<long>(val);
                    m_data[i][(m_t[i]+t)%m_time_spans[i]][n] = val;
                }

            }

            m_graph[i]->axisY()->setRange(m_min_ranges[i], m_max_ranges[i]);
            //update the 3D graph
            //std::cout << m_chunk_size[i]<< std::endl;

            update3Dgraph(i);
            m_t[i] += m_chunk_size[i];
            //std::cout << "get" << std::endl;

        }
    }




    /*update the 2D graph*/
    //update2Dgraph();



}

void MainWindow::update3Dgraph(unsigned i)
{
    //std::cout << "hey" << std::endl;
    if((i==0)?ui->checkBox_3D->isChecked():ui->checkBox_3D_2->isChecked())
    {
        //allocate a new surface (in x axis)
        QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
        dataArray->reserve(static_cast<int>(m_time_spans[i]));
        for (uint t = 0 ; t < m_time_spans[i] ; t++)
        {
            // Example with m_t = 1   ;  chunk size = 3   ;   time span = 8;
            // | o | o | o | o | o | o | o | o |
            //     ^-----------^ ^
            //       new chunk   '-----------,
            // so rr_ind need to start here -' so the rightest data is the oldest
            unsigned rr_ind = (m_t[i]+m_chunk_size[i]+t) % m_time_spans[i];//round robin index
            //std::cout << rr_ind<< std::endl;
            //std::cout << t<< std::endl;
            QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(static_cast<int>(m_nb_channels[i]));
            for (uint n = 0; n < m_nb_channels[i]; n++)
            {
                //draw a point at the coresponding point
                //the data  array is use as a roundrobin
                float x = static_cast<float>(m_time_stamps[i][rr_ind]);
                //std::cout << n << std::endl;
                //std::cout << m_data[i][rr_ind].size() << std::endl;
                float y = static_cast<float>(m_data[i][rr_ind][n]);
                float z = static_cast<float>(n);
                (*newRow)[static_cast<int>(n)].setPosition(QVector3D(x, y, z));
            }
            *dataArray << newRow;
        }

        //update the time window of the graph
        float min_time = static_cast<float>(m_time_stamps[i][(m_t[i]+m_chunk_size[i]) % m_time_spans[i]]);
        float max_time = static_cast<float>(m_time_stamps[i][(m_t[i]+m_chunk_size[i]-1) % m_time_spans[i]]);
        //std::cout << min_time << " " << max_time << std::endl;
        m_graph[i]->axisX()->setRange(min_time, max_time);
        m_proxy_chart3D[i]->resetArray(dataArray);

    }

}


void MainWindow::update2Dgraph()
{
    /*for(uint i =0; i<2; i++)
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
    }*/
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
