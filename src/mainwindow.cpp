#include "include/mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox_stream->addItem("LSL Stream", 0);
    ui->lineEdi_stream->setText("OTB");
    ui->spinBox_channelMin->setValue(m_Zmin);
    ui->spinBox_channelMax->setValue(m_Zmax);
    ui->label_nb_channels->setNum(m_ZnbSample);
    ui->label_rangeY->setText("[ "+QString::number(m_Ymin) +" ; "+QString::number(m_Ymax) +"] ");

    connect(ui->pushButton_connect_stream, SIGNAL (released()), this, SLOT (connect_stream()));
    connect(ui->comboBox_stream, SIGNAL (currentIndexChanged(int)), this, SLOT(change_stream(int)));
    connect(ui->spinBox_channelMin, SIGNAL (valueChanged(int)), this, SLOT(changeChannelsRange()));
    connect(ui->spinBox_channelMax, SIGNAL (valueChanged(int)), this, SLOT(changeChannelsRange()));
    connect(ui->radioButton_3Dheatmap, SIGNAL (clicked()), this, SLOT(changeHeatMapRange()));
    connect(ui->pushButton_scanStream, SIGNAL (released()), this, SLOT(scanStream()));

    //init the timer object used to update the graph
    QObject::connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(1);
    m_timer.start();

    createDataArray();
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
    m_chart2D = new QtCharts::QChart();
    //store the m_data to a graphic object
    for (uint i = 0 ; i < m_ZnbSample; i++)
    {
        QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D);
        for (unsigned int j = 0; j < m_XnbSample; j++)
        {
            float x = (j>m_Xmax)?m_Xmax:(j<m_Xmin)?m_Xmin:j;
            float y = m_data[i][j] ;//+ (i==1&&j==3)?m_counter%50:0;
            y = (y>m_Ymax)?m_Ymax:(y<m_Ymin)?m_Ymin:y;
            series->append(QPointF(x,y));
        }
        series->setName(QString::number(i));
        m_chart2D->addSeries(series);
    }

    //set up the chart
    m_chart2D->createDefaultAxes();
    m_chart2D->axes(Qt::Horizontal).first()->setRange(m_Xmin, m_Xmax);
    m_chart2D->axes(Qt::Vertical).first()->setRange(m_Ymin, m_Ymax);

    QtCharts::QChartView *chartView = new QtCharts::QChartView(m_chart2D);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->chart()->legend()->hide();
    ui->horizontalLayout_2D->addWidget(chartView);
}

void MainWindow::init3DGraph()
{
    //store the m_data to a graphic object
    QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
    dataArray->reserve(m_ZnbSample);
    for (int i = 0; i < m_ZnbSample; i++)
    {
        QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_XnbSample);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
        int index =0;
        float z = (i>m_Zmax)?m_Zmax:(i<m_Zmin)?m_Zmin:i;
        for (int j = 0 ; j < m_XnbSample ; j++)
        {
            float x = (j>m_Xmax)?m_Xmax:(j<m_Xmin)?m_Xmin:j;
            float y = m_data[i][j];
            (*newRow)[index++].setPosition(QVector3D(x, y, z));
        }
        *dataArray << newRow;
    }

    m_proxy_chart3D = new QtDataVisualization::QSurfaceDataProxy();
    m_proxy_chart3D->resetArray(dataArray);

    //set up the graph and the chart
    m_chart3D = new QtDataVisualization::QSurface3DSeries(m_proxy_chart3D);
    m_chart3D->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurface);
    m_chart3D->setFlatShadingEnabled(true);
    m_chart3D->setItemLabelVisible(false);
    //m_chart3D->setMeshSmooth(true);

    m_graph = new QtDataVisualization::Q3DSurface();
    QWidget *container = QWidget::createWindowContainer(m_graph);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->horizontalLayout_3D->addWidget(container);
    m_graph->axisX()->setRange(m_Xmin, m_Xmax);
    m_graph->axisY()->setRange(m_Ymin, m_Ymax);
    m_graph->axisZ()->setRange(m_Zmin, m_Zmax);//static_cast<int>(m_channels_min_ranges[i])-1, m_channels_max_ranges[i]+1);
    m_graph->addSeries(m_chart3D);
    m_graph->axisX()->setLabelAutoRotation(30);
    m_graph->axisY()->setLabelAutoRotation(90);
    m_graph->axisZ()->setLabelAutoRotation(30);
    m_graph->setReflection(false);
    m_graph->setReflectivity(0);
    m_graph->setOrthoProjection(true);

    //great a gradient to see the heat map
    QLinearGradient gr;
    gr.setColorAt(0.0, Qt::black);
    gr.setColorAt(0.33, Qt::blue);
    gr.setColorAt(0.67, Qt::red);
    gr.setColorAt(1.0, Qt::yellow);
    m_graph->seriesList().at(0)->setBaseGradient(gr);
    m_graph->seriesList().at(0)->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);

    m_graph->scene()->activeCamera()->setCameraPosition(0, 90);
    m_graph->scene()->activeCamera()->setZoomLevel(100);
    m_graph->setHorizontalAspectRatio(1);

}

void MainWindow::connect_stream()
{
    ui->pushButton_connect_stream->setDisabled(true);
    ui->lineEdi_stream->setDisabled(true);
    ui->comboBox_stream->setDisabled(true);

//    if(ui->comboBox_stream->currentIndex()==0)

    std::string stream_label = ui->lineEdi_stream->text().toStdString();
    std::cout << "Trying to connect to \"" << stream_label << "\""<< std::endl;
    std::vector<lsl::stream_info> results = lsl::resolve_stream("name",stream_label);
    m_inlet = new lsl::stream_inlet(results[0]);


}

void MainWindow::handleTimeout()
{
    //if an lsl stream is connected

    if(m_inlet)
    {
        std::vector<std::vector<short>> chunk;
        std::vector<double> time_stamps;
        if(m_inlet->pull_chunk(chunk,time_stamps))//get the sample
        {
            m_chunk_size = chunk.size();
            if(m_chunk_size/m_mean_span > m_XnbSample)
                std::cout << "warning data overflow" << std::endl;
            //if the number of channels has changed
            if(m_ZnbSample != chunk[0].size())
            {
                m_ZnbSample = chunk[0].size();
                m_Zmax = m_ZnbSample;
                createDataArray();
                m_counter = 0;
                ui->spinBox_channelMax->setValue(m_Zmax);
            }

            //average the data and store it
            int m = 0;
            int i = m_counter%m_XnbSample;
            std::vector<short> sample(chunk[0].size());//mean sample vector
            for(int t = 0; t < m_chunk_size; t++)
            {
                for(int n = 0; n < chunk[t].size(); n++)
                    sample[n] += chunk[t][n];
                m++;

                if(m%m_mean_span==0)//if enoight data was summed then divide to obtain the mean
                {
                    for(int n = 0; n < chunk[t].size(); n++)
                    {
                        sample[n] /= m;
                        if(sample[n] > m_Ymax)
                            m_Ymax = sample[n];
                        if(sample[n] < m_Ymin)
                            m_Ymin = sample[n];

                        m_data[n][i] = sample[n];
                        sample[n]=0;
                    }
                    m = 0;
                    i = (++i)%m_XnbSample;
                }
            }
            if(m!=0)//if the number of data received was not a multiple of the mean span
            {
                for(int n = 0; n < chunk[0].size(); n++)
                    m_data[n][i] = sample[n]/m;
                i++;
                i = i%m_XnbSample;
            }
            m_counter =i;//increment the counter of the number of new data sampled
            m_graph->axisY()->setRange(m_Ymin, m_Ymax);//reajust the Y ranges
            m_chart2D->axes(Qt::Vertical).first()->setRange(m_Ymin, m_Ymax);

            update3Dgraph();
            update2Dgraph();

            //update stream info
            ui->label_nb_channels->setNum(m_ZnbSample);
            ui->label_rangeY->setText("[ "+QString::number(m_Ymin) +" ; "+QString::number(m_Ymax) +"] ");
        }
    }

}

void MainWindow::createDataArray()
{
    m_data.clear();
    std::vector<float> v;
    for(uint z = 0; z < m_ZnbSample ; z++)
    {
        m_data.push_back(v);
        for(uint x = 0; x < m_XnbSample ; x++)
            m_data[z].push_back(0);
    }
}

void MainWindow::update3Dgraph()
{
    if(ui->groupBox_3D->isChecked())
    {
        if(ui->radioButton_3Dtemporal->isChecked())
        {

            m_graph->axisZ()->setRange(m_Zmin, m_Zmax);
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            dataArray->reserve(m_ZnbSample);
            for (int i = m_Zmin; i < m_Zmax; i++)
            {
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_XnbSample);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
                int index =0;
                float z = (i>m_Zmax)?m_Zmax:(i<m_Zmin)?m_Zmin:i;
                for (int j = 0 ; j < m_XnbSample ; j++)
                {
                    int rrb = (j+m_counter)%m_XnbSample;
                    float x = (j>m_Xmax)?m_Xmax:(j<m_Xmin)?m_Xmin:j;
                    float y = m_data[i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax)?m_Ymax:(y<m_Ymin)?m_Ymin:y;
                    (*newRow)[index++].setPosition(QVector3D(x, y, z));
                }
                *dataArray << newRow;
            }

            //m_proxy_chart3D = new QtDataVisualization::QSurfaceDataProxy();
            m_proxy_chart3D->resetArray(dataArray);
        }
        else
        {
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            int chmin=ui->spinBox_heatmapChmin->value();
            int chmax=ui->spinBox_heatmapChmax->value();
            int zwidth=ui->spinBox_heatmapZWidthSize->value();
            int xwidth=(chmax-chmin)/zwidth;
            m_graph->axisZ()->setRange(0, zwidth);
            m_graph->axisX()->setRange(0, xwidth);

            int rrb = (m_counter-1)%m_XnbSample;

            dataArray->reserve(zwidth);
            for (int i = 0; i < zwidth; i++)
            {
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(xwidth);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
                int index =0;
                float z = (i>zwidth)?zwidth:(i<0)?0:i;
                for (int j = 0 ; j < xwidth ; j++)
                {
                    int k = chmin + i*xwidth + j;
                    float x = (j>xwidth)?xwidth:(j<0)?0:j;
                    float y = m_data[k][rrb];//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax)?m_Ymax:(y<m_Ymin)?m_Ymin:y;
                    (*newRow)[index++].setPosition(QVector3D(x, y, z));
                }
                *dataArray << newRow;
            }

            //m_proxy_chart3D = new QtDataVisualization::QSurfaceDataProxy();
            m_proxy_chart3D->resetArray(dataArray);
        }

    }

}


void MainWindow::update2Dgraph()
{
    if(ui->groupBox_2D->isChecked())
    {
        m_chart2D->removeAllSeries();
        //store the m_data to a graphic object
        if(ui->radioButton_all->isChecked())
        {
            for (uint i = 0 ; i < m_ZnbSample; i++)
            {
                QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D);
                for (unsigned int j = 0; j < m_XnbSample; j++)
                {
                    unsigned long rrb = (j+m_counter)%m_XnbSample;
                    float x = (j>m_Xmax)?m_Xmax:(j<m_Xmin)?m_Xmin:j;
                    float y = m_data[i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax)?m_Ymax:(y<m_Ymin)?m_Ymin:y;
                    series->append(QPointF(x,y));
                }
                series->setName(QString::number(i));
                m_chart2D->addSeries(series);
            }
            //ui->radioButton_solo->setChecked(false);
        }
        else
        {
            //ui->radioButton_all->setChecked(false);
            int i = ui->spinBox_channelNum->value();
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D);
            for (unsigned int j = 0; j < m_XnbSample; j++)
            {
                unsigned long rrb = (j+m_counter)%m_XnbSample;
                float x = (j>m_Xmax)?m_Xmax:(j<m_Xmin)?m_Xmin:j;
                float y = m_data[i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                y = (y>m_Ymax)?m_Ymax:(y<m_Ymin)?m_Ymin:y;
                series->append(QPointF(x,y));
            }
            series->setName(QString::number(i));
            m_chart2D->addSeries(series);
        }
    }
}

void MainWindow::change_stream(int index)
{
    if(index>-1)
    {
        std::cout << index << std::endl;
        ui->lineEdi_stream->setText(ui->comboBox_stream->currentText());
        ui->lineEdit_stream_port->setText("");
        ui->lineEdit_stream_port->setPlaceholderText("None");
        ui->label_type->setText(QString::fromStdString(m_results[index].type()));
        ui->label_nb_ch->setNum(m_results[index].channel_count());
        ui->label_format->setText(QString::fromStdString(channel_format_str[m_results[index].channel_format()]));
        ui->label_host->setText(QString::fromStdString(m_results[index].hostname()));
    }

}

void MainWindow::changeChannelsRange()
{
    m_Zmax = ui->spinBox_channelMax->value();
    m_Zmin = ui->spinBox_channelMin->value();
    m_graph->axisZ()->setRange(m_Zmin, m_Zmax);
}

void MainWindow::changeHeatMapRange()
{
    ui->spinBox_heatmapChmin->setValue(0);
    ui->spinBox_heatmapChmax->setValue(m_ZnbSample);
    int w = static_cast<int>(sqrt(m_ZnbSample));
    if(m_ZnbSample%w==0)
        ui->spinBox_heatmapZWidthSize->setValue(w);
    else {
        while(m_ZnbSample%w!=0){w++;}
        ui->spinBox_heatmapZWidthSize->setValue(w);
    }
}

void MainWindow::scanStream()
{
    m_results = lsl::resolve_streams();
    if(m_results.size()>0)
     {
         ui->label_type->setText(QString::fromStdString(m_results[0].type()));
         ui->label_nb_ch->setNum(m_results[0].channel_count());
         ui->label_format->setText(QString::fromStdString(channel_format_str[m_results[0].channel_format()]));
         ui->label_host->setText(QString::fromStdString(m_results[0].hostname()));
     }

    ui->comboBox_stream->clear();
    int i=0;
    for (auto& stream: m_results)
        ui->comboBox_stream->addItem( QString::fromStdString(stream.name()), i++);


}

MainWindow::~MainWindow()
{
    delete ui;
}
