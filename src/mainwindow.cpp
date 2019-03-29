#include "include/mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);

    ui->horizontalSlider_timeSpan->setValue(0);
    ui->horizontalSlider_mean_span->setValue(0);

    ui->treeWidget_streams->setColumnCount(2);
    ui->treeWidget_streams->setHeaderLabels(QStringList() << tr("Streams")<< tr("Infos"));

    connect(ui->pushButton_connect_stream, SIGNAL (released()), this, SLOT (connect_stream()));
    connect(ui->comboBox_stream, SIGNAL (currentIndexChanged(int)), this, SLOT (change_stream(int)));

    connect(ui->groupBox_3D, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->groupBox_2D, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_all, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_solo, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->radioButton_solo, SIGNAL (clicked()), this, SLOT (updateGUI()));
    connect(ui->horizontalSlider_YmaxDisp, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));
    connect(ui->horizontalSlider_YminDisp, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));

    connect(ui->spinBox_channelNum, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));
    connect(ui->spinBox_heatmapChmax, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));
    connect(ui->spinBox_heatmapChmin, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));
    connect(ui->spinBox_heatmapZWidthSize, SIGNAL (valueChanged(int)), this, SLOT (updateGUI()));

    connect(ui->pushButton_scanStream, SIGNAL (released()), this, SLOT(scanStream()));
    connect(ui->treeWidget_streams, SIGNAL(itemClicked(QTreeWidgetItem *, int)), SLOT(updateGUI()) );

    connect(ui->horizontalSlider_mean_span, SIGNAL (valueChanged(int)), this, SLOT(changeMeanSpan()));
        //init the timer object used to update the graph
    QObject::connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(10);
    m_timer.start();

    // Set the colors from the light theme as default ones
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
    pal.setColor(QPalette::WindowText, QRgb(0x404044));
    qApp->setPalette(pal);



}

void MainWindow::initDataArray(unsigned index)
{
    m_counter.push_back(0);
    m_Xmin.push_back(0);
    m_Xmax.push_back(1000);
    m_XnbSample.push_back(1000);;//time span

    m_Ymin.push_back(-100);
    m_Ymax.push_back(100);
    m_YminDisp.push_back(-100);
    m_YmaxDisp.push_back(100);

    m_Zmin.push_back(0);
    m_Zmax.push_back(m_inlet[index]->get_channel_count());
    m_ZnbSample.push_back(static_cast<unsigned>(m_inlet[index]->get_channel_count()));//nb channels

    m_chunk_size.push_back(0);
    m_mean_span.push_back(100);

    m_stream2Dstate.push_back(1);
    m_stream3Dstate.push_back(1);

    std::vector<bool> v(static_cast<unsigned>(m_inlet[index]->get_channel_count()), true);
    m_channelIsValid.push_back(v);

    std::vector<std::vector<double>> d;
    m_data.push_back(d);
    createDataArray(index);
}

void MainWindow::create2Dgraph(unsigned index, std::string name)
{
    m_chart2D.push_back(new QtCharts::QChart());
    //store the m_data to a graphic object
    for (unsigned i = 0 ; i < m_ZnbSample[index]; i++)
    {
        QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D[index]);
        for (unsigned int j = 0; j < m_XnbSample[index]; j++)
        {
            double x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
            double y = m_data[index][i][j] ;//+ (i==1&&j==3)?m_counter%50:0;
            y = (y>m_Ymax[index])?m_Ymax[index]:(y<m_YminDisp[index])?m_YminDisp[index]:y;
            series->append(QPointF(x,y));
        }
        series->setName(QString::number(i));
        m_chart2D[index]->addSeries(series);
    }

    //set up the chart
    m_chart2D[index]->createDefaultAxes();
    /*m_chart2D[index]->axes(Qt::Horizontal).first()->setRange(m_Xmin[index], m_Xmax[index]);
    m_chart2D[index]->axes(Qt::Vertical).first()->setRange(m_Ymin[index], m_Ymax[index]);*/

    QtCharts::QChartView *chartView = new QtCharts::QChartView(m_chart2D[index]);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->chart()->legend()->hide();
    chartView->setRenderHint(QPainter::Antialiasing);
    QDockWidget *dock = new QDockWidget(tr("LSL Stream: ")+QString::fromStdString(name),this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(chartView);
    dock->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    dock->setMinimumWidth(800);
    dock->setMinimumHeight(500);
    //dock->setFloating(true);
    this->addDockWidget(Qt::RightDockWidgetArea,dock);

    //ui->scrollArea_2D->//addWidget(chartView);
}



void MainWindow::create3Dgraph(unsigned index, std::string name)
{
    //store the m_data to a graphic object
    QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
    dataArray->reserve(static_cast<int>(m_ZnbSample[index]));
    for (unsigned i = 0; i < m_ZnbSample[index]; i++)
    {
        QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(static_cast<int>(m_XnbSample[index]));
        int ind = 0;
        double z = (i>m_Zmax[index])?m_Zmax[index]:(i<m_Zmin[index])?m_Zmin[index]:i;
        for (unsigned j = 0 ; j < m_XnbSample[index] ; j++)
        {
            double x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
            double y = m_data[index][i][j];
            (*newRow)[ind++].setPosition(QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
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
    dock->setMinimumHeight(500);
    //dock->setFloating(true);
    this->addDockWidget(Qt::LeftDockWidgetArea,dock);

    m_graph[index]->axisX()->setRange(static_cast<float>(m_Xmin[index]), static_cast<float>(m_Xmax[index]));
    m_graph[index]->axisY()->setRange(static_cast<float>(m_YminDisp[index]), static_cast<float>(m_YmaxDisp[index]));
    m_graph[index]->axisZ()->setRange(static_cast<float>(m_Zmin[index]), static_cast<float>(m_Zmax[index]));
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
    ui->comboBox_stream->clear();
    for(unsigned i = 0; i < m_results.size(); i++)
    {
        std::string stream_label = m_results[i].name();
        if(isStreamsTicked(stream_label))
        {
            std::cout << "Trying to connect to \"" << stream_label << "\""<< std::endl;
            std::vector<lsl::stream_info> results = lsl::resolve_stream("name",stream_label);
            m_inlet.push_back(new lsl::stream_inlet(results[0]));
            initDataArray(m_stream_count);
            create3Dgraph(m_stream_count,stream_label);
            create2Dgraph(m_stream_count,stream_label);
            ui->comboBox_stream->addItem( QString::fromStdString(stream_label), i);
        }

        m_stream_count++;
    }
}

void MainWindow::handleTimeout()
{
    //if an lsl stream is connected
    for(unsigned index = 0; index < m_inlet.size(); index++)
    {
        switch (m_inlet[index]->info().channel_format())
        {
        case lsl::cf_int16:
                storeChunk_int16(index);
            break;
        case lsl::cf_float32:
                storeChunk_float(index);
            break;
        default:
            //TODO other format
            break;
        }

        update3Dgraph(index);
        update2Dgraph(index);

    }

}

void MainWindow::storeChunk_int16(unsigned index)
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
            m_ZnbSample[index] = static_cast<unsigned>(chunk[0].size());
            m_Zmax[index] = m_ZnbSample[index];
            createDataArray(index);
            m_counter[index] = 0;
        }

        //average the data and store it
        unsigned m = 0;
        unsigned i = m_counter[index]%m_XnbSample[index];
        std::vector<short> sample(chunk[0].size());//mean sample vector
        for(unsigned t = 0; t < m_chunk_size[index]; t++)
        {
            std::cout << "   ";
            for(unsigned n = 0; n < chunk[t].size(); n++)
            {
                sample[n] += chunk[t][n];
                std::cout << chunk[t][n] << "  ";
            }
            std::cout << std::endl;
            m++;

            if(m%m_mean_span[index]==0)//if enoight data was summed then divide to obtain the mean
            {
                std::cout << "-> ";
                for(unsigned n = 0; n < chunk[t].size(); n++)
                {
                    sample[n] /= m;
                    if(sample[n] > m_Ymax[index])
                        m_Ymax[index] = sample[n];
                    if(sample[n] < m_Ymin[index])
                        m_Ymin[index] = sample[n];

                    m_data[index][n][i] = sample[n];
                    std::cout << m_data[index][n][i] << "  ";
                    sample[n]=0;
                }
                std::cout << std::endl;
                m = 0;
                i++;
                i = i%m_XnbSample[index];
            }
        }
        if(m!=0)//if the number of data received was not a multiple of the mean span
        {
            for(unsigned n = 0; n < chunk[0].size(); n++)
            {
                m_data[index][n][i] = static_cast<double>(sample[n]/static_cast<float>(m));
                std::cout <<  m_data[index][n][i] << "  ";
            }
            std::cout << std::endl;
            i++;
            i = i%m_XnbSample[index];
        }
        m_counter[index] =i;//increment the counter of the number of new data sampled

        //m_chart2D[index]->axes(Qt::Vertical).first()->setRange(m_Ymin[index], m_Ymax[index]);
    }
}

void MainWindow::storeChunk_float(unsigned index)
{
    std::vector<std::vector<float>> chunk;
    std::vector<double> time_stamps;
    if(m_inlet[index]->pull_chunk(chunk,time_stamps))//get the sample
    {
        m_chunk_size[index] = chunk.size();
        if(m_chunk_size[index]/m_mean_span[index] > m_XnbSample[index])
            std::cout << "warning data overflow" << std::endl;
        //if the number of channels has changed
        if(m_ZnbSample[index] != chunk[0].size())
        {
            m_ZnbSample[index] = static_cast<unsigned>(chunk[0].size());
            m_Zmax[index] = m_ZnbSample[index];
            createDataArray(index);
            m_counter[index] = 0;
        }

        //average the data and store it
        unsigned m = 0;
        unsigned i = m_counter[index]%m_XnbSample[index];
        std::vector<float> sample(chunk[0].size());//mean sample vector
        for(unsigned t = 0; t < m_chunk_size[index]; t++)
        {
            std::cout << "   ";
            for(unsigned n = 0; n < chunk[t].size(); n++)
            {
                sample[n] += chunk[t][n];
                std::cout << chunk[t][n] << "  ";
            }
            std::cout << std::endl;
            m++;

            if(m%m_mean_span[index]==0)//if enoight data was summed then divide to obtain the mean
            {
                std::cout << "-> ";
                for(unsigned n = 0; n < chunk[t].size(); n++)
                {
                    sample[n] /= m;
                    if(static_cast<double>(sample[n]) > m_Ymax[index])
                        m_Ymax[index] = static_cast<double>(sample[n]);
                    if(static_cast<double>(sample[n])< m_Ymin[index])
                        m_Ymin[index] = static_cast<double>(sample[n]);
                    m_data[index][n][i] = static_cast<double>(sample[n]);
                    std::cout << m_data[index][n][i] << "  ";
                    sample[n]=0;
                }
                std::cout << std::endl;
                m = 0;
                i++;
                i = i%m_XnbSample[index];
            }
        }
        if(m!=0)//if the number of data received was not a multiple of the mean span
        {
            for(unsigned n = 0; n < chunk[0].size(); n++)
            {
                m_data[index][n][i] = static_cast<double>(sample[n]/static_cast<float>(m));
                std::cout << m_data[index][n][i] << "  ";
            }
            std::cout << std::endl;
            i++;
            i = i%m_XnbSample[index];
        }
        m_counter[index] =i;//increment the counter of the number of new data sampled
        //m_chart2D[index]->axes(Qt::Vertical).first()->setRange(m_Ymin[index], m_Ymax[index]);
    }
}

void MainWindow::createDataArray(unsigned index)
{
    m_data[index].clear();
    std::vector<double> v;
    for(unsigned z = 0; z < m_ZnbSample[index] ; z++)
    {
        m_data[index].push_back(v);
        for(unsigned x = 0; x < m_XnbSample[index] ; x++)
            m_data[index][z].push_back(0);
    }
}

void MainWindow::update3Dgraph(unsigned index)
{
    if(m_stream3Dstate[index])
    {
        if(m_stream3Dstate[index]==2)
        {
            m_graph[index]->axisY()->setRange(static_cast<float>(m_YminDisp[index]), static_cast<float>(m_YmaxDisp[index]));//reajust the Y ranges
            QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
            int chmin=ui->spinBox_heatmapChmin->value();
            int chmax=ui->spinBox_heatmapChmax->value()+1;
            int zwidth=ui->spinBox_heatmapZWidthSize->value();
            int xwidth=(chmax-chmin)/zwidth;
            m_graph[index]->axisZ()->setRange(0, zwidth);
            m_graph[index]->axisX()->setRange(0, xwidth);

            unsigned rrb = (m_counter[index]-1)%m_XnbSample[index];

            dataArray->reserve(zwidth);
            for (int i = 0; i < zwidth; i++)
            {
                QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(xwidth);//static_cast<int>(m_channels_max_ranges[i]-m_channels_min_ranges[i]));
                int ind =0;
                float z = (i>zwidth)?zwidth:(i<0)?0:i;
                for (int j = 0 ; j < xwidth ; j++)
                {
                    unsigned k = static_cast<unsigned>(chmin + i*xwidth + j);
                    double x = (j>xwidth)?xwidth:(j<0)?0:j;
                    double y = m_data[index][k][rrb];
                    y = (m_channelIsValid[index][k]==false)?0:(y>m_YmaxDisp[index])?m_YmaxDisp[index]:(y<m_YminDisp[index])?m_YminDisp[index]:y;
                    (*newRow)[ind++].setPosition(QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                }
                *dataArray << newRow;
            }
            m_proxy_chart3D[index]->resetArray(dataArray);
        }

    }

}


void MainWindow::update2Dgraph(unsigned index)
{
    if(m_stream2Dstate[index])
    {
        m_chart2D[index]->removeAllSeries();
        //store the m_data to a graphic object
        if(m_stream2Dstate[index]==1)
        {
            for (uint i = 0 ; i < m_ZnbSample[index]; i++)
            {
                QtCharts::QLineSeries *series = new QtCharts::QLineSeries();
                for (unsigned int j = 0; j < m_XnbSample[index]; j++)
                {
                    unsigned long rrb = (j+m_counter[index])%m_XnbSample[index];
                    double x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
                    double y = m_data[index][i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                    y = (m_channelIsValid[index][i]==false)?0:(y>m_Ymax[index])?m_Ymax[index]:(y<m_YminDisp[index])?m_YminDisp[index]:y;
                    series->append(qreal(x),qreal(y));
                }
                series->setName(QString::number(i));
                m_chart2D[index]->addSeries(series);

            }
            m_chart2D[index]->createDefaultAxes();
            //ui->radioButton_solo->setChecked(false);
        }
        else
        {
            //ui->radioButton_all->setChecked(false);
            unsigned i = static_cast<unsigned>(ui->spinBox_channelNum->value());
            QtCharts::QSplineSeries *series = new QtCharts::QSplineSeries(m_chart2D[index]);
            for (unsigned  j = 0; j < m_XnbSample[index]; j++)
            {
                unsigned long rrb = (j+m_counter[index])%m_XnbSample[index];
                double x = (j>m_Xmax[index])?m_Xmax[index]:(j<m_Xmin[index])?m_Xmin[index]:j;
                double y = m_data[index][i][rrb] ;//+ (i==1&&j==3)?m_counter%50:0;
                y = (m_channelIsValid[index][i]==false)?0:(y>m_Ymax[index])?m_Ymax[index]:(y<m_YminDisp[index])?m_YminDisp[index]:y;
                series->append(qreal(x),qreal(y));
            }
            series->setName(QString::number(i));
            m_chart2D[index]->addSeries(series);
        }
    }
}

void MainWindow::change_stream(int index)
{
    m_selectedStream=index;
    ui->horizontalSlider_YminDisp->setValue(static_cast<int>(m_YminDisp[static_cast<unsigned>(index)]));
    ui->horizontalSlider_YmaxDisp->setValue(static_cast<int>(m_YmaxDisp[static_cast<unsigned>(index)]));
    changeHeatMapRange();
    updateGUI();
}

bool MainWindow::isStreamsTicked(std::string name)
{
    QString qname = QString::fromStdString(name);
    for (int i = 0; i< ui->treeWidget_streams->topLevelItemCount();i++)
    {
        if(QString::compare(qname,ui->treeWidget_streams->topLevelItem(i)->text(0))==0)
            if(ui->treeWidget_streams->topLevelItem(i)->checkState(0))
                return true;
    }
    return  false;
}

int MainWindow::StreamsIndex(QString name)
{
    for (int i = 0; i< ui->treeWidget_streams->topLevelItemCount();i++)
    {
        if(QString::compare(name,ui->treeWidget_streams->topLevelItem(i)->text(0))==0)
            if(ui->treeWidget_streams->topLevelItem(i)->checkState(0))
                return i;
    }
    return  -1;
}

void MainWindow::changeChannelsRange()
{
}

void MainWindow::changeHeatMapRange()
{
    int index = m_selectedStream;
    if(index!=-1)
    {
        unsigned ind = static_cast<unsigned>(index);
        ui->spinBox_heatmapChmin->setValue(0);
        ui->spinBox_heatmapChmax->setValue(static_cast<int>(m_ZnbSample[ind])-1);
        unsigned w = static_cast<unsigned>(sqrt(m_ZnbSample[ind]));
        if(m_ZnbSample[ind]%w==0 && w != 0)
            ui->spinBox_heatmapZWidthSize->setValue(static_cast<int>(w));
        else {
            while(m_ZnbSample[ind]%++w!=0);
            ui->spinBox_heatmapZWidthSize->setValue(static_cast<int>(w));        }

    }
}

void MainWindow::scanStream()
{
    //ui->pushButton_scanStream->setEnabled(false);
    m_results = lsl::resolve_streams();

    ui->treeWidget_streams->clear();
    for (auto& stream: m_results)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_streams);
        item->setText(0, QString::fromStdString(stream.name()));
        item->setCheckState(0, Qt::Checked);
        //item->setFlags(item->flags() | Qt::ItemIsAutoTristate);



        QTreeWidgetItem *subitem_type = new QTreeWidgetItem(item);
        subitem_type->setText(0, "Type");
        subitem_type->setText(1, QString::fromStdString(m_results[0].type()));

        QTreeWidgetItem *subitem_nbChan = new QTreeWidgetItem(item);
        subitem_nbChan->setText(0, "Channels");
        subitem_nbChan->setText(1, QString::number(m_results[0].channel_count()));
        //std::vector<QTreeWidgetItem> test(m_results[0].channel_count());

        for(int n =0 ; n < m_results[0].channel_count(); n++)
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
    int index = m_selectedStream;
    if(index!=-1)
    {
        unsigned ind = static_cast<unsigned>(index);
        m_XnbSample[ind] = static_cast<unsigned>(ui->horizontalSlider_timeSpan->value());
        m_Xmax[ind] = m_XnbSample[ind];
        createDataArray(ind);
        m_counter[ind] = 0;
        ui->label_timeSpan->setNum(m_Xmax[ind]);
    }
}
void MainWindow::changeMeanSpan()
{
    int index = m_selectedStream;
    if(index!=-1)
    {
        unsigned ind = static_cast<unsigned>(index);
        m_mean_span[ind] = static_cast<unsigned>(ui->horizontalSlider_mean_span->value());
        ui->label_meanSpan->setNum(static_cast<int>(m_mean_span[ind]));
    }
}

void MainWindow::updateGUI()
{
    int index = m_selectedStream;
    if(index!=-1)
    {
        unsigned ind = static_cast<unsigned>(index);
        if(ui->groupBox_3D->isChecked())
                m_stream3Dstate[ind]=2;
        else
            m_stream3Dstate[ind]=0;

        if(ui->groupBox_2D->isChecked())
        {
            if(ui->radioButton_all->isChecked())
                m_stream2Dstate[ind]=1;
            else
                m_stream2Dstate[ind]=2;
        }
        else
            m_stream2Dstate[ind]=0;

        ui->horizontalSlider_mean_span->setValue(static_cast<int>(m_mean_span[ind]));
        ui->label_meanSpan->setNum(static_cast<int>(m_mean_span[ind]));

        m_YmaxDisp[ind] = ui->horizontalSlider_YmaxDisp->value();
        m_YminDisp[ind] = ui->horizontalSlider_YminDisp->value();

        ui->horizontalSlider_YmaxDisp->setMaximum(static_cast<int>(m_Ymax[ind]));
        ui->horizontalSlider_YmaxDisp->setMinimum(static_cast<int>(m_YminDisp[ind])+1);
        ui->horizontalSlider_YminDisp->setMaximum(static_cast<int>(m_YmaxDisp[ind])-1);
        ui->horizontalSlider_YminDisp->setMinimum(static_cast<int>(m_Ymin[ind]));

        ui->label_YmaxDisp->setNum(m_YmaxDisp[ind]);
        ui->label_YminDisp->setNum(m_YminDisp[ind]);

        if(ui->spinBox_channelNum->value() >= static_cast<int>(m_ZnbSample[ind]))
            ui->spinBox_channelNum->setValue(static_cast<int>(m_ZnbSample[ind])-1);
        if(ui->spinBox_channelNum->value() < 0)
            ui->spinBox_channelNum->setValue(0);

        if(ui->spinBox_heatmapChmax->value() >= static_cast<int>(m_ZnbSample[ind]))
            ui->spinBox_heatmapChmax->setValue(static_cast<int>(m_ZnbSample[ind])-1);
        if(ui->spinBox_heatmapChmax->value() <= ui->spinBox_heatmapChmin->value())
            ui->spinBox_heatmapChmax->setValue(ui->spinBox_heatmapChmin->value()+1);

        if(ui->spinBox_heatmapChmin->value() < 0)
            ui->spinBox_heatmapChmin->setValue(0);
        if(ui->spinBox_heatmapChmin->value() >= ui->spinBox_heatmapChmax->value())
            ui->spinBox_heatmapChmin->setValue(ui->spinBox_heatmapChmax->value()-1);

        if(ui->spinBox_heatmapZWidthSize->value() >= static_cast<int>(m_ZnbSample[ind]))
            ui->spinBox_heatmapZWidthSize->setValue(static_cast<int>(m_ZnbSample[ind])-1);
        if(ui->spinBox_heatmapZWidthSize->value() < 1)
            ui->spinBox_heatmapZWidthSize->setValue(1);


    }
    if(m_stream_count>0)
    {
        for(unsigned i = 0 ; i < static_cast<unsigned>(ui->treeWidget_streams->topLevelItemCount()); i++)
        {
            for (unsigned c=0;c < static_cast<unsigned>(ui->treeWidget_streams->topLevelItem(static_cast<int>(i))->child(1)->childCount()) ;c++)
            {
                int ind = StreamsIndex(ui->treeWidget_streams->topLevelItem(static_cast<int>(i))->text(0));
                m_channelIsValid[i][c] = (ui->treeWidget_streams->topLevelItem(ind)->child(1)->child(static_cast<int>(c))->checkState(1)==2)?true:false;
                //std::cout << c << " " <<  m_channelIsValid[i][c] << std::endl;
            }
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
