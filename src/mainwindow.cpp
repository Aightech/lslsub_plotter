/**
 * \file mainwindow.cpp
 * \brief TODO.
 * \author Alexis Devillard
 * \version 1.0
 * \date 03 march 2019
 */
#include "include/mainwindow.h"
#include "ui_mainwindow.h"
#include<string>
#include <fstream>
#include <sstream>


MainWindow::MainWindow(char *path, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //find the root path of the pluggin
#ifdef WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA( NULL, buffer, MAX_PATH );
    char * c = strstr(buffer, "lslsub_plotter")+15;
    *c = '\0';
    root_path = buffer;
#else
    root_path = path;
#endif

    this->setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);
    ui->treeWidget_streams->setColumnCount(2);
    ui->treeWidget_streams->setHeaderLabels(QStringList() << tr("Streams")<< tr("Infos"));

    connect(ui->pushButton_scanStream, SIGNAL (released()), this, SLOT(scanStream()));
    connect(ui->pushButton_plot_line, SIGNAL (released()), this, SLOT(plot_line()));
    connect(ui->pushButton_plot_heatmap, SIGNAL (released()), this, SLOT(plot_heatmap()));

    std::ifstream source;                    // build a read-Stream
    source.open("conf.cfg", std::ios_base::in);  // open data
    if (source)
    {
        std::string line;
        std::getline(source, line);
        std::istringstream in(line);
        int a;
        double b;
        in >> a;
        ui->spinBox_minVal->setValue(a);
        in >> a;
        ui->spinBox_maxVal->setValue(a);
        in >> a;
        ui->spinBox_heatmapChmin->setValue(a);
        in >> a;
        ui->spinBox_heatmapWidth->setValue(a);
        in >> a;
        ui->spinBox_heatmapHeight->setValue(a);
        in >> a;
        ui->spinBox_n->setValue(a);
        in >> a;
        ui->spinBox_order->setValue(a);
        in >> b;
        ui->doubleSpinBox_lowpass->setValue(b);
        in >> b;
        ui->doubleSpinBox_highpass->setValue(b);
        in >> b;
        ui->doubleSpinBox_stopband_low->setValue(b);
        in >> b;
        ui->doubleSpinBox__stopband_high->setValue(b);
    }


    // Set the colors from the light theme as default ones
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
    pal.setColor(QPalette::WindowText, QRgb(0x404044));
    qApp->setPalette(pal);
}

void MainWindow::plot_line()
{
    if(m_results.size() > (unsigned) ui->comboBox_stream->currentIndex())
    {
#ifdef WIN32
        std::string command = "cd " + root_path + " && start cmd /k python script/plot_line.py";
#else
        std::string command = "python3 script/plot_line.py";
#endif

        command += " " + m_results[ui->comboBox_stream->currentIndex()].name();
        command += " " + std::to_string(ui->spinBox_heatmapChmin->value());
        if(ui->radioButton_all->isChecked())
        {
            command += " " + std::to_string(ui->spinBox_heatmapWidth->value());
            command += " " + std::to_string(ui->spinBox_heatmapHeight->value());
        }
        else
            command += " 1 1";
        command += " " + std::to_string(ui->spinBox_minVal->value());
        command += " " + std::to_string(ui->spinBox_maxVal->value());
        if(ui->radioButton_all->isChecked())
            command += " " + std::to_string(ui->checkBox->isChecked()?0:1);
        else
            command += " 1";

        command += " " + std::to_string(ui->checkBox_scaleAuto->isChecked()?1:0);
        command += " " + std::to_string(ui->spinBox_n->value());

        command += " " + std::to_string(ui->checkBox_fft->isChecked()?1:0);

        command += " " + std::to_string(ui->spinBox_order->value());
        command += " " + std::to_string(ui->checkBox_lowpass->isChecked()?ui->doubleSpinBox_lowpass->value():-1);
        command += " " + std::to_string(ui->checkBox_highpass->isChecked()?ui->doubleSpinBox_highpass->value():-1);
        command += " " + std::to_string(ui->checkBox_stopband->isChecked()?ui->doubleSpinBox_stopband_low->value():-1);
        command += " " + std::to_string(ui->checkBox_stopband->isChecked()?ui->doubleSpinBox__stopband_high->value():-1);

        command += " " + std::to_string(ui->checkBox_AC->isChecked()?1:0);
        command += " " + std::to_string(ui->checkBox_Rectified->isChecked()?1:0);

        //stream_name first_ch nrows ncols min_v max_v one_plot auto_scale n fft_enabled order lowpass_f highpass_f stopband_low_f stopband_high_f AC_enabled rectified_enabled

        command += " &";



        std::cout << command << std::endl;
        std::system(command.c_str());
    }
}

void MainWindow::plot_heatmap()
{
    if(m_results.size() > (unsigned) ui->comboBox_stream->currentIndex())
    {
#ifdef WIN32
        std::string command = "cd " + root_path + " && start cmd /k python script/plot_heatmap.py";
#else
        std::string command = "python3 script/plot_heatmap.py";
#endif
        command += " " + m_results[ui->comboBox_stream->currentIndex()].name();
        command += " " + std::to_string(ui->spinBox_heatmapChmin->value());
        command += " " + std::to_string(ui->spinBox_heatmapWidth->value());
        command += " " + std::to_string(ui->spinBox_heatmapHeight->value());
        command += " " + std::to_string(ui->spinBox_minVal->value());
        command += " " + std::to_string(ui->spinBox_maxVal->value());
        std::cout << command << std::endl;
        std::system(command.c_str());
    }
}

void MainWindow::scanStream()
{
    //ui->pushButton_scanStream->setEnabled(false);
    m_results = lsl::resolve_streams();

    ui->treeWidget_streams->clear();
    ui->comboBox_stream->clear();
    for (auto& stream: m_results)
    {
        ui->comboBox_stream->addItem(QString::fromStdString(stream.name()));
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_streams);
        item->setText(0, QString::fromStdString(stream.name()));
        item->setCheckState(0, Qt::Checked);


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



MainWindow::~MainWindow()
{
    std::ofstream myfile ("conf.cfg",std::ios::trunc);
    if (myfile.is_open())
    {
        myfile << ui->spinBox_minVal->value() << " ";
        myfile << ui->spinBox_maxVal->value() << " ";
        myfile << ui->spinBox_heatmapChmin->value() << " ";
        myfile << ui->spinBox_heatmapWidth->value() << " ";
        myfile << ui->spinBox_heatmapHeight->value() << " ";
        myfile << ui->spinBox_n->value() << " ";
        myfile << ui->spinBox_order->value() << " ";
        myfile << ui->doubleSpinBox_lowpass->value() << " ";
        myfile << ui->doubleSpinBox_highpass->value() << " ";
        myfile << ui->doubleSpinBox_stopband_low->value() << " ";
        myfile << ui->doubleSpinBox__stopband_high->value() << " ";
    }
    myfile.close();
    delete ui;
}

