#include "include/mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);
    ui->comboBox_stream->addItem("LSL Stream", 0);
    ui->lineEdi_stream->setText("OTB");
    ui->spinBox_channelMin->setValue(0);
    ui->spinBox_channelMax->setValue(0);

    ui->horizontalSlider_timeSpan->setValue(0);
    ui->horizontalSlider_mean_span->setValue(0);

    //ui->dockWidget_3->
    //QTreeWidget *tree = new QTreeWidget();
    ui->treeWidget_streams->setColumnCount(2);
    ui->treeWidget_streams->setHeaderLabels(QStringList() << tr("Streams")<< tr("Infos"));

    connect(ui->pushButton_connect_stream, SIGNAL (released()), this, SLOT (connect_stream()));
    connect(ui->comboBox_stream, SIGNAL (currentIndexChanged(int)), this, SLOT (change_stream(int)));
    connect(ui->spinBox_channelMin, SIGNAL (valueChanged(int)), this, SLOT (changeChannelsRange()));
    connect(ui->spinBox_channelMax, SIGNAL (valueChanged(int)), this, SLOT (changeChannelsRange()));

    connect(ui->radioButton_3Dheatmap, SIGNAL (clicked()), this, SLOT (changeHeatMapRange()));
    connect(ui->radioButton_3Dheatmap, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_3Dtemporal, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->groupBox_3D, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->groupBox_2D, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_all, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_solo, SIGNAL (clicked()), this, SLOT (updateGUI()));

    connect(ui->pushButton_scanStream, SIGNAL (released()), this, SLOT(scanStream()));
    connect(ui->treeWidget_streams, SIGNAL(itemClicked(QTreeWidgetItem *, int)), SLOT(updateGUI()) );
    //connect(ui->horizontalSlider_mean_span, SIGNAL (sliderReleased()), this, SLOT(changeMeanSpan()));
    //connect(ui->horizontalSlider_timeSpan, SIGNAL (sliderReleased()), this, SLOT(changeXnbSample()));

    //init the timer object used to update the graph
    QObject::connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(1);
    m_timer.start();

    // Set the colors from the light theme as default ones
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
    pal.setColor(QPalette::WindowText, QRgb(0x404044));
    qApp->setPalette(pal);



}

void MainWindow::initDataArray(int index)
{
    m_counter.push_back(0);
    m_Xmin.push_back(0);
    m_Xmax.push_back(1000);
    m_XnbSample.push_back(1000);;//time span

    m_Ymin.push_back(-100);
    m_Ymax.push_back(100);

    m_Zmin.push_back(0);
    m_Zmax.push_back(m_inlet[index]->get_channel_count());
    m_ZnbSample.push_back(m_inlet[index]->get_channel_count());//nb channels

    m_chunk_size.push_back(0);
    m_mean_span.push_back(100);

    m_stream2Dstate.push_back(1);
    m_stream3Dstate.push_back(1);

    std::vector<std::vector<float>> d;
    m_data.push_back(d);
    createDataArray(index);
}

void MainWindow::create2Dgraph(int index, std::string name)
{
    m_chart2D.push_back(new QtCharts::QChart());
    //store the m_data to a graphic object
    for (uint i = 0 ; i < m_ZnbSample[index]; i++)
    {
        QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[index]);
        for (unsigned int j = 0; j < m_XnbSample[index]; j++)
        {
            float x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
            float y = m_data[index][i][j] ;//+ (i==1&&j==3)?m_counter%50:0;
            y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_Ymin[index])?m_Ymin[index]:y;
            series->append(QPointF(x,y));
        }
        series->setName(QString::number(i));
        m_chart2D[index]->addSeries(series);
    }

    //set up the chart
    m_chart2D[index]->createDefaultAxes();
    m_chart2D[index]->axes(Qt::Horizontal).first()->setRange(m_Xmin[index], m_Xmax[index]);
    m_chart2D[index]->axes(Qt::Vertical).first()->setRange(m_Ymin[index], m_Ymax[index]);

    QtCharts::QChartView *chartView = new QtCharts::QChartView(m_chart2D[index]);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->chart()->legend()->hide();
    QDockWidget *dock = new QDockWidget(tr("Customers"),this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(chartView);
    dock->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    this->addDockWidget(Qt::RightDockWidgetArea,dock);
    //ui->scrollArea_2D->//addWidget(chartView);
}



void MainWindow::create3Dgraph(int index, std::string name)
{
    //store the m_data to a graphic object
    QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
    dataArray->reserve(m_ZnbSample[index]);
    for (int i = 0; i < m_ZnbSample[index]; i++)
    {
        QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_XnbSample[index]);
        int ind = 0;
        float z = (i>m_Zmax[index])?m_Zmax[index]:(i<m_Zmin[index])?m_Zmin[index]:i;
        for (int j = 0 ; j < m_XnbSample[index] ; j++)
        {
            float x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
            float y = m_data[index][i][j];
            (*newRow)[ind++].setPosition(QVector3D(x, y, z));
        }
        *dataArray << newRow;
    }
    m_proxy_chart3D.push_back(new QtDataVisualization::QSurfaceDataProxy());
    m_proxy_chart3D[index]->resetArray(dataArray);

    //set up the graph and the chart
    m_chart3D.push_back(new QtDataVisualization::QSurface3DSeries(m_proxy_chart3D[index]));
    m_chart3D[index]->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurface);
    m_chart3D[index]->setFlatShadingEnabled(true);
    m_chart3D[index]->setItemLabelVisible(false);
    //m_chart3D->setMeshSmooth(true);

    m_graph.push_back(new QtDataVisualization::Q3DSurface());
    QWidget *container = QWidget::createWindowContainer(m_graph[index]);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QDockWidget *dock = new QDockWidget(tr("LSL Stream: ")+QString::fromStdString(name),this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(container);
    dock->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    dock->setMinimumWidth(500);
    this->addDockWidget(Qt::LeftDockWidgetArea,dock);
    //ui->scrollArea_3D->
    m_graph[index]->axisX()->setRange(m_Xmin[index], m_Xmax[index]);
    m_graph[index]->axisY()->setRange(m_Ymin[index], m_Ymax[index]);
    m_graph[index]->axisZ()->setRange(m_Zmin[index], m_Zmax[index]);//static_cast<int>(m_channels_min_ranges[i])-1, m_channels_max_ranges[i]+1);
    m_graph[index]->addSeries(m_chart3D[index]);
    m_graph[index]->axisX()->setLabelAutoRotation(30);
    m_graph[index]->axisY()->setLabelAutoRotation(90);
    m_graph[index]->axisZ()->setLabelAutoRotation(30);
    m_graph[index]->setReflection(false);
    m_graph[index]->setReflectivity(0);
    m_graph[index]->setOrthoProjection(true);
    std::cout << "23"<< std::endl;
    //great a gradient to see the heat map
    QLinearGradient gr;
    gr.setColorAt(0.0, Qt::black);
    gr.setColorAt(0.33, Qt::blue);
    gr.setColorAt(0.67, Qt::red);
    gr.setColorAt(1.0, Qt::yellow);
    m_graph[index]->seriesList().at(0)->setBaseGradient(gr);
    m_graph[index]->seriesList().at(0)->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);

    m_graph[index]->scene()->activeCamera()->setCameraPosition(0, 90);
    m_graph[index]->scene()->activeCamera()->setZoomLevel(100);
    m_graph[index]->setHorizontalAspectRatio(1);
}

void MainWindow::connect_stream()
{
    //ui->pushButton_connect_stream->setDisabled(true);
    //ui->lineEdi_stream->setDisabled(true);
    //ui->comboBox_stream->setDisabled(true);

//    if(ui->comboBox_stream->currentIndex()==0)

    std::string stream_label = ui->lineEdi_stream->text().toStdString();
    std::cout << "Trying to connect to \"" << stream_label << "\""<< std::endl;
    std::vector<lsl::stream_info> results = lsl::resolve_stream("name",stream_label);
    m_inlet.push_back(new lsl::stream_inlet(results[0]));
    initDataArray(m_stream_count);
    create3Dgraph(m_stream_count,stream_label);
    create2Dgraph(m_stream_count,stream_label);

    m_stream_count++;
}

void MainWindow::handleTimeout()
{
    //if an lsl stream is connected
    for(int index = 0; index < m_inlet.size(); index++)
    {
        std::vector<std::vector<short>> chunk;
        std::vector<double> time_stamps;
        if(m_inlet[index]->pull_chunk(chunk,time_stamps))//get the sample
        {
            m_chunk_size[index] = chunk.size();
            if(m_chunk_size[index]/m_mean_span[index] > m_XnbSample[index])
                std::cout << "warning data overflow" << std::endl;
            //if the number of channels has changed
            if(m_ZnbSample[index] != chunk[0].size())
            {
                m_ZnbSample[index] = chunk[0].size();
                m_Zmax[index] = m_ZnbSample[index];
                createDataArray(index);
                m_counter[index] = 0;
                ui->spinBox_channelMax->setValue(m_Zmax[index]);
            }

            //average the data and store it
            int m = 0;
            int i = m_counter[index]%m_XnbSample[index];
            std::vector<short> sample(chunk[0].size());//mean sample vector
            for(int t = 0; t < m_chunk_size[index]; t++)
            {
                for(int n = 0; n < chunk[t].size(); n++)
                    sample[n] += chunk[t][n];
                m++;

                if(m%m_mean_span[index]==0)//if enoight data was summed then divide to obtain the mean
                {
                    for(int n = 0; n < chunk[t].size(); n++)
                    {
                        sample[n] /= m;
                        if(sample[n] > m_Ymax[index])
                            m_Ymax[index] = sample[n];
                        if(sample[n] < m_Ymin[index])
                            m_Ymin[index] = sample[n];

                        m_data[index][n][i] = sample[n];
                        sample[n]=0;
                    }
                    m = 0;
                    i = (++i)%m_XnbSample[index];
                }
            }
            if(m!=0)//if the number of data received was not a multiple of the mean span
            {
                for(int n = 0; n < chunk[0].size(); n++)
                    m_data[index][n][i] = sample[n]/m;
                i++;
                i = i%m_XnbSample[index];
            }
            m_counter[index] =i;//increment the counter of the number of new data sampled
            m_graph[index]->axisY()->setRange(m_Ymin[index], m_Ymax[index]);//reajust the Y ranges
            m_chart2D[index]->axes(Qt::Vertical).first()->setRange(m_Ymin[index], m_Ymax[index]);

            update3Dgraph(index);
            update2Dgraph(index);
        }
    }

}

void MainWindow::createDataArray(int index)
{
    m_data[index].clear();
    std::vector<float> v;
    for(uint z = 0; z < m_ZnbSample[index] ; z++)
    {
        m_data[index].push_back(v);
        for(uint x = 0; x < m_XnbSample[index] ; x++)
            m_data[index][z].push_back(0);
    }
}

void MainWindow::update3Dgraph(int index)
{
    if(m_stream3Dstate[index])
    {
        if(m_stream3Dstate[index]==1)
        {

            m_graph[index]->axisZ()->setRange(m_Zmin[index], m_Zmax[index]);
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            dataArray->reserve(m_ZnbSample[index]);
            for (int i = m_Zmin[index]; i < m_Zmax[index]; i++)
            {
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_XnbSample[index]);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
                int ind =0;
                float z = (i>m_Zmax[index])?m_Zmax[index]:(i<m_Zmin[index])?m_Zmin[index]:i;
                for (int j = 0 ; j < m_XnbSample[index] ; j++)
                {
                    int rrb = (j+m_counter[index])%m_XnbSample[index];
                    float x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
                    float y = m_data[index][i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_Ymin[index])?m_Ymin[index]:y;
                    (*newRow)[ind++].setPosition(QVector3D(x, y, z));
                }
                *dataArray << newRow;
            }

            //m_proxy_chart3D = new QtDataVisualization::QSurfaceDataProxy();
            m_proxy_chart3D[index]->resetArray(dataArray);
        }
        else
        {
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            int chmin=ui->spinBox_heatmapChmin->value();
            int chmax=ui->spinBox_heatmapChmax->value();
            int zwidth=ui->spinBox_heatmapZWidthSize->value();
            int xwidth=(chmax-chmin)/zwidth;
            m_graph[index]->axisZ()->setRange(0, zwidth);
            m_graph[index]->axisX()->setRange(0, xwidth);

            int rrb = (m_counter[index]-1)%m_XnbSample[index];

            dataArray->reserve(zwidth);
            for (int i = 0; i < zwidth; i++)
            {
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(xwidth);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
                int ind =0;
                float z = (i>zwidth)?zwidth:(i<0)?0:i;
                for (int j = 0 ; j < xwidth ; j++)
                {
                    int k = chmin + i*xwidth + j;
                    float x = (j>xwidth)?xwidth:(j<0)?0:j;
                    float y = m_data[index][k][rrb];//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_Ymin[index])?m_Ymin[index]:y;
                    (*newRow)[ind++].setPosition(QVector3D(x, y, z));
                }
                *dataArray << newRow;
            }

            //m_proxy_chart3D = new QtDataVisualization::QSurfaceDataProxy();
            //m_proxy_chart3D->removeRows(0,)
            m_proxy_chart3D[index]->resetArray(dataArray);
        }

    }

}


void MainWindow::update2Dgraph(int index)
{
    if(m_stream2Dstate[index])
    {
        m_chart2D[index]->removeAllSeries();
        //store the m_data to a graphic object
        if(m_stream2Dstate[index]==1)
        {
            for (uint i = 0 ; i < m_ZnbSample[index]; i++)
            {
                QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[index]);
                for (unsigned int j = 0; j < m_XnbSample[index]; j++)
                {
                    unsigned long rrb = (j+m_counter[index])%m_XnbSample[index];
                    float x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
                    float y = m_data[index][i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                    y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_Ymin[index])?m_Ymin[index]:y;
                    series->append(QPointF(x,y));
                }
                series->setName(QString::number(i));
                m_chart2D[index]->addSeries(series);
            }
            //ui->radioButton_solo->setChecked(false);
        }
        else
        {
            //ui->radioButton_all->setChecked(false);
            int i = ui->spinBox_channelNum->value();
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[index]);
            for (unsigned int j = 0; j < m_XnbSample[index]; j++)
            {
                unsigned long rrb = (j+m_counter[index])%m_XnbSample[index];
                float x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
                float y = m_data[index][i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_Ymin[index])?m_Ymin[index]:y;
                series->append(QPointF(x,y));
            }
            series->setName(QString::number(i));
            m_chart2D[index]->addSeries(series);
        }
    }
}

void MainWindow::change_stream(int index)
{
    if(index>-1)
    {
        //std::cout << index << std::endl;
        ui->lineEdi_stream->setText(ui->comboBox_stream->currentText());
        //ui->lineEdit_stream_port->setText("");
        //ui->lineEdit_stream_port->setPlaceholderText("None");
        ui->label_type->setText(QString::fromStdString(m_results[index].type()));
        ui->label_nb_ch->setNum(m_results[index].channel_count());
        ui->label_format->setText(QString::fromStdString(channel_format_str[m_results[index].channel_format()]));
        ui->label_host->setText(QString::fromStdString(m_results[index].hostname()));
    }

}

int MainWindow::selectedStream()
{
    int index = -1;
    if(ui->treeWidget_streams->currentItem())
    {
        for(int i = 0; i < m_inlet.size(); i++)
            if(!QString::compare(ui->treeWidget_streams->currentItem()->text(0),QString::fromStdString(m_inlet[i]->info().name())))
                index=i;
    }
    else
    {
        std::cout << "please select a stream" << std::endl;
    }
    return  index;
}

void MainWindow::changeChannelsRange()
{
    int index = selectedStream();
    if(index!=-1)
    {
        m_Zmax[index] = ui->spinBox_channelMax->value();
        m_Zmin[index] = ui->spinBox_channelMin->value();
        m_graph[index]->axisZ()->setRange(m_Zmin[index], m_Zmax[index]);
    }
}

void MainWindow::changeHeatMapRange()
{
    int index = selectedStream();
    if(index!=-1)
    {
        ui->spinBox_heatmapChmin->setValue(0);
        ui->spinBox_heatmapChmax->setValue(m_ZnbSample[index]);
        int w = static_cast<int>(sqrt(m_ZnbSample[index]));
        if(m_ZnbSample[index]%w==0 && w != 0)
            ui->spinBox_heatmapZWidthSize->setValue(w);
        else {
            while(m_ZnbSample[index]%++w!=0);
            ui->spinBox_heatmapZWidthSize->setValue(w);
        }

    }
}

void MainWindow::scanStream()
{
    //ui->pushButton_scanStream->setEnabled(false);
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

    ui->treeWidget_streams->clear();
    for (auto& stream: m_results)
    {
        ui->comboBox_stream->addItem( QString::fromStdString(stream.name()), i++);
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_streams);
        item->setText(0, QString::fromStdString(stream.name()));
        item->setCheckState(0, Qt::Checked);
        //item->setFlags(item->flags() | Qt::ItemIsAutoTristate);



        QTreeWidgetItem *subitem_type = new QTreeWidgetItem(item);
        subitem_type->setText(0, "Type");
        subitem_type->setText(1, QString::fromStdString(m_results[0].type()));

        QTreeWidgetItem *subitem_nbChan = new QTreeWidgetItem(item);
        subitem_nbChan->setText(0, "Nb Channels");
        subitem_nbChan->setText(1, QString::number(m_results[0].channel_count()));
        //std::vector<QTreeWidgetItem> test(m_results[0].channel_count());

        for(int n =0 ; n < 100; n++)
        {
            //std::cout << n << std::endl;
            QTreeWidgetItem* subitem_nbChan_channel = new QTreeWidgetItem(subitem_nbChan);
            subitem_nbChan_channel->setText(0, QString::number(n));
            subitem_nbChan_channel->setCheckState(1, Qt::Checked);
        }

        QTreeWidgetItem *subitem_format = new QTreeWidgetItem(item);
        subitem_format->setText(0, "Format");
        subitem_format->setText(1, QString::fromStdString(channel_format_str[m_results[0].channel_format()]));

        QTreeWidgetItem *subitem_host = new QTreeWidgetItem(item);
        subitem_host->setText(0, "Host");
        subitem_host->setText(1, QString::fromStdString(m_results[0].hostname()));

        QTreeWidgetItem *subitem_rate = new QTreeWidgetItem(item);
        subitem_rate->setText(0, "Rate");
        subitem_rate->setText(1, QString::number(m_results[0].nominal_srate()));
    }


}

void MainWindow::changeXnbSample()
{
    int index = selectedStream();
    if(index!=-1)
    {
        m_XnbSample[index] = ui->horizontalSlider_timeSpan->value();
        m_Xmax[index] = m_XnbSample[index];
        createDataArray(index);
        m_counter[index] = 0;
        ui->label_timeSpan->setNum(m_Xmax[index]);
    }
}
void MainWindow::changeMeanSpan()
{
    int index = selectedStream();
    if(index!=-1)
    {
        m_mean_span[index] = ui->horizontalSlider_mean_span->value();
        ui->label_meanSpan->setNum((int)m_mean_span[index]);
    }
}

void MainWindow::updateGUI()
{
    int index = selectedStream();
    if(index!=-1)
    {
        if(ui->groupBox_3D->isChecked())
        {
            if(ui->radioButton_3Dtemporal->isChecked())
                m_stream3Dstate[index]=1;
            else
                m_stream3Dstate[index]=2;
        }
        else
            m_stream3Dstate[index]=0;

        if(ui->groupBox_2D->isChecked())
        {
            if(ui->radioButton_all->isChecked())
                m_stream2Dstate[index]=1;
            else
                m_stream2Dstate[index]=2;
        }
        else
            m_stream2Dstate[index]=0;

        ui->spinBox_channelMin->setValue(m_Zmin[index]);
        ui->spinBox_channelMax->setValue(m_Zmax[index]);
        changeHeatMapRange();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
