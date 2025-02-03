/**
 * \file mainwindow.cpp
 * \brief TODO.
 * \author Alexis Devillard
 * \version 1.0
 * \date 03 march 2019
 */
#include "include/mainwindow.h"
#include "gnuplot-iostream.h"
#include "ui_mainwindow.h"
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <fftw3.h>

MainWindow::MainWindow(char *path, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //find the root path of the pluggin
#ifdef WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    char *c = strstr(buffer, "lslsub_plotter") + 15;
    *c = '\0';
    root_path = buffer;
#else
    root_path = path;
#endif

    this->setDockOptions(AnimatedDocks | AllowTabbedDocks | AllowNestedDocks);
    ui->treeWidget_streams->setColumnCount(2);
    ui->treeWidget_streams->setHeaderLabels(QStringList()
                                            << tr("Streams") << tr("Infos"));

    connect(ui->pushButton_scanStream, SIGNAL(released()), this,
            SLOT(scanStream()));
    connect(ui->pushButton_plot_line, SIGNAL(released()), this,
            SLOT(plot_line()));
    connect(ui->pushButton_plot_heatmap, SIGNAL(released()), this,
            SLOT(plot_heatmap()));

    std::ifstream source;                       // build a read-Stream
    source.open("conf.cfg", std::ios_base::in); // open data
    if(source)
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

void
MainWindow::plot_line()
{
    if(m_results.size() <= (unsigned)ui->comboBox_stream->currentIndex())
    {
        std::cerr << "No stream selected." << std::endl;
        return;
    }

    // Capture UI parameters.
    lsl::stream_info info = m_results[ui->comboBox_stream->currentIndex()];
    double fixed_yMin = ui->spinBox_minVal->value();
    double fixed_yMax = ui->spinBox_maxVal->value();
    int first_ch = ui->spinBox_heatmapChmin->value(); // starting channel index

    int nrows = 1, ncols = 1;
    int one_plot = 1;
    bool solo = !ui->radioButton_all->isChecked();
    if(!solo)
    {
        nrows = ui->spinBox_heatmapWidth->value();
        ncols = ui->spinBox_heatmapHeight->value();
        one_plot = (ui->checkBox->isChecked() ? 1 : 0);
    }

    bool auto_scale = ui->checkBox_scaleAuto->isChecked();
    int max_points = ui->spinBox_n->value();
    bool fft_enabled = ui->checkBox_fft->isChecked();
    int order = ui->spinBox_order->value();
    double lowpass_f = ui->checkBox_lowpass->isChecked()
                           ? ui->doubleSpinBox_lowpass->value()
                           : -1;
    double highpass_f = ui->checkBox_highpass->isChecked()
                            ? ui->doubleSpinBox_highpass->value()
                            : -1;
    double stopband_low_f = ui->checkBox_stopband->isChecked()
                                ? ui->doubleSpinBox_stopband_low->value()
                                : -1;
    double stopband_high_f = ui->checkBox_stopband->isChecked()
                                 ? ui->doubleSpinBox__stopband_high->value()
                                 : -1;
    bool AC_enabled = ui->checkBox_AC->isChecked();
    bool rectified_enabled = ui->checkBox_Rectified->isChecked();

    std::cout << "Plotting stream: " << info.name() << std::endl;

    std::thread(
        [info, fixed_yMin, fixed_yMax, first_ch, nrows, ncols, one_plot, solo,
         auto_scale, max_points, fft_enabled, order, lowpass_f, highpass_f,
         stopband_low_f, stopband_high_f, AC_enabled, rectified_enabled]()
        {
            // Create inlet.
            lsl::stream_inlet inlet(info);
            int total_channels = info.channel_count();
            if(first_ch >= total_channels)
            {
                std::cerr << "Invalid starting channel index." << std::endl;
                return;
            }
            if(solo)
                total_channels = first_ch + 1;
            int num_plot_channels = total_channels - first_ch;

            // FIFO buffers for channels to plot.
            std::vector<std::deque<std::pair<double, double>>> fifo(
                num_plot_channels);
            // Filter state vectors (for simple exponential filters).
            std::vector<double> prev_lp(num_plot_channels, 0.0);
            std::vector<double> prev_hp(num_plot_channels, 0.0);

            std::vector<std::vector<double>> chunk;
            std::vector<double> timestamps;
            double timestamps_offset = -1;

            // Initialise gnuplot.
            Gnuplot gp;
            gp << "set terminal wxt persist\n";
            double prev_sample_time = -1.0; // for dt computation
            std::vector<double> notch_x1(num_plot_channels, 0.0);
            std::vector<double> notch_x2(num_plot_channels, 0.0);
            std::vector<double> notch_y1(num_plot_channels, 0.0);
            std::vector<double> notch_y2(num_plot_channels, 0.0);
            std::vector<double> prev_dc(num_plot_channels, 0.0);
            while(true)
            {

                if(inlet.pull_chunk(chunk, timestamps))
                {
                    if(!timestamps.empty() && timestamps_offset < 0)
                        timestamps_offset = timestamps[0];

                    // Process each sample in the chunk.
                    for(size_t j = 0; j < timestamps.size(); ++j)
                    {
                        double t = timestamps[j] - timestamps_offset;
                        // Compute dt: use a default value if no previous sample exists.
                        double dt =
                            (prev_sample_time < 0 ? 0.1 : t - prev_sample_time);
                        prev_sample_time = t;

                        // Process channels starting from first_ch.
                        for(int ch = first_ch; ch < total_channels; ch++)
                        {
                            int idx = ch - first_ch;
                            if(ch < static_cast<int>(chunk[j].size()))
                            {
                                double sample = chunk[j][ch];
                                double filtered = sample;

                                // --- Low–pass filtering (first–order IIR) ---
                                if(lowpass_f > 0)
                                {
                                    // Compute RC from cutoff frequency.
                                    double RC = 1.0 / (2 * M_PI * lowpass_f);
                                    double alpha = dt / (RC + dt);
                                    filtered = prev_lp[idx] +
                                               alpha * (sample - prev_lp[idx]);
                                    prev_lp[idx] = filtered;
                                }

                                // --- High–pass filtering (via subtracting low–pass estimate) ---
                                if(highpass_f > 0)
                                {
                                    double RC = 1.0 / (2 * M_PI * highpass_f);
                                    double alpha = dt / (RC + dt);
                                    double lp = prev_hp[idx] +
                                                alpha * (sample - prev_hp[idx]);
                                    prev_hp[idx] = lp;
                                    filtered = sample - lp;
                                }

                                // --- Stopband filtering (Notch filter implementation) ---
                                if(stopband_low_f > 0 && stopband_high_f > 0)
                                {
                                    // Center frequency and bandwidth.
                                    double f0 =
                                        (stopband_low_f + stopband_high_f) /
                                        2.0;
                                    double bandwidth =
                                        stopband_high_f - stopband_low_f;
                                    if(bandwidth <= 0)
                                        bandwidth = 1; // avoid division by zero
                                    double Q = f0 / bandwidth;
                                    double fs =
                                        (dt > 0
                                             ? 1.0 / dt
                                             : 1000); // approximate sample rate
                                    double omega0 = 2 * M_PI * f0 / fs;
                                    double alpha_notch = sin(omega0) / (2 * Q);

                                    // Biquad notch coefficients.
                                    double b0 = 1;
                                    double b1 = -2 * cos(omega0);
                                    double b2 = 1;
                                    double a0 = 1 + alpha_notch;
                                    double a1 = -2 * cos(omega0);
                                    double a2 = 1 - alpha_notch;

                                    double x0 = filtered;
                                    double y0 = (b0 * x0 + b1 * notch_x1[idx] +
                                                 b2 * notch_x2[idx] -
                                                 a1 * notch_y1[idx] -
                                                 a2 * notch_y2[idx]) /
                                                a0;
                                    // Update filter state.
                                    notch_x2[idx] = notch_x1[idx];
                                    notch_x1[idx] = x0;
                                    notch_y2[idx] = notch_y1[idx];
                                    notch_y1[idx] = y0;
                                    filtered = y0;
                                }

                                // --- AC filtering (remove DC component by subtracting a running average) ---
                                if(AC_enabled)
                                {
                                    double dc_alpha =
                                        0.01; // smoothing factor; adjust as needed
                                    // Update and subtract running average.
                                    prev_dc[idx] =
                                        dc_alpha * filtered +
                                        (1 - dc_alpha) * prev_dc[idx];
                                    filtered = filtered - prev_dc[idx];
                                }

                                // --- Rectification ---
                                if(rectified_enabled)
                                    filtered = std::abs(filtered);

                                // (FFT filtering is applied later on the entire FIFO.)

                                // Push the filtered sample into the FIFO for channel idx.
                                fifo[idx].push_back({t, filtered});
                                while(fifo[idx].size() >
                                      static_cast<size_t>(max_points))
                                    fifo[idx].pop_front();
                            }
                        }
                    }

                    // --- FFT filtering (applied per channel if FIFO is full) ---
                    if(fft_enabled)
                    {
                        for(int idx = 0; idx < num_plot_channels; idx++)
                        {
                            if(fifo[idx].size() ==
                               static_cast<size_t>(max_points))
                            {
                                int N = max_points;
                                // Copy FIFO data into a vector.
                                std::vector<double> in(N);
                                int k = 0;
                                for(const auto &p : fifo[idx])
                                    in[k++] = p.second;

                                int N_complex = N / 2 + 1;
                                std::vector<fftw_complex> fft_out(N_complex);
                                fftw_plan plan_forward = fftw_plan_dft_r2c_1d(
                                    N, in.data(), fft_out.data(),
                                    FFTW_ESTIMATE);
                                fftw_execute(plan_forward);
                                fftw_destroy_plan(plan_forward);

                                // Estimate sampling frequency from FIFO time span.
                                double dt_total = fifo[idx].back().first -
                                                  fifo[idx].front().first;
                                double fs =
                                    (dt_total > 0 ? (N - 1) / dt_total : 1000);
                                double df = fs / N;
                                // Use highpass and lowpass parameters as FFT passband bounds.
                                double f_low =
                                    (highpass_f > 0 ? highpass_f : 0);
                                double f_high =
                                    (lowpass_f > 0 ? lowpass_f : fs / 2);

                                // Zero out FFT bins outside the passband.
                                for(int i = 0; i < N_complex; i++)
                                {
                                    double freq = i * df;
                                    if(freq < f_low || freq > f_high)
                                    {
                                        fft_out[i][0] = 0;
                                        fft_out[i][1] = 0;
                                    }
                                }

                                std::vector<double> filtered_signal(N);
                                fftw_plan plan_backward = fftw_plan_dft_c2r_1d(
                                    N, fft_out.data(), filtered_signal.data(),
                                    FFTW_ESTIMATE);
                                fftw_execute(plan_backward);
                                fftw_destroy_plan(plan_backward);

                                // Normalize the inverse FFT result.
                                for(int i = 0; i < N; i++)
                                    filtered_signal[i] /= N;

                                // Replace FIFO data with the FFT–filtered values.
                                k = 0;
                                for(auto &p : fifo[idx])
                                    p.second = filtered_signal[k++];
                            }
                        }
                    }

                    try
                    {
                        // Plotting.
                        if(solo || one_plot == 0)
                        {
                            // Shared plot: plot all channels in one graph.
                            if(auto_scale)
                            {
                                double global_min =
                                    std::numeric_limits<double>::max();
                                double global_max =
                                    std::numeric_limits<double>::lowest();
                                for(const auto &deq : fifo)
                                {
                                    for(const auto &p : deq)
                                    {
                                        if(p.second < global_min)
                                            global_min = p.second;
                                        if(p.second > global_max)
                                            global_max = p.second;
                                    }
                                }
                                gp << "set yrange [" << global_min << ":"
                                   << global_max << "]\n";
                            }
                            else
                            {
                                gp << "set yrange [" << fixed_yMin << ":"
                                   << fixed_yMax << "]\n";
                            }

                            gp << "plot ";
                            for(int i = 0; i < num_plot_channels; i++)
                            {
                                if(i > 0)
                                    gp << ", ";
                                gp << "'-' with lines title 'Ch "
                                   << (first_ch + i) << "'";
                            }
                            gp << "\n";

                            for(int i = 0; i < num_plot_channels; i++)
                            {
                                std::vector<std::pair<double, double>> data(
                                    fifo[i].begin(), fifo[i].end());
                                gp.send1d(data);
                            }
                        }
                        else
                        {
                            // Separate plots: use multiplot layout.
                            gp << "set multiplot layout " << nrows << ", "
                               << ncols << "\n";
                            for(int i = 0; i < num_plot_channels; i++)
                            {
                                if(auto_scale && !fifo[i].empty())
                                {
                                    double local_min =
                                        std::numeric_limits<double>::max();
                                    double local_max =
                                        std::numeric_limits<double>::lowest();
                                    for(const auto &p : fifo[i])
                                    {
                                        if(p.second < local_min)
                                            local_min = p.second;
                                        if(p.second > local_max)
                                            local_max = p.second;
                                    }
                                    gp << "set yrange [" << local_min << ":"
                                       << local_max << "]\n";
                                }
                                else
                                {
                                    gp << "set yrange [" << fixed_yMin << ":"
                                       << fixed_yMax << "]\n";
                                }
                                gp << "plot '-' with lines title 'Ch "
                                   << (first_ch + i) << "'\n";
                                std::vector<std::pair<double, double>> data(
                                    fifo[i].begin(), fifo[i].end());
                                gp.send1d(data);
                            }
                            gp << "unset multiplot\n";
                        }
                    }
                    catch(const lsl::timeout_error &)
                    {
                        std::cerr
                            << "\n\n\n\n\n\n\n\n\n\n\nTimeout error: no data "
                               "received."
                            << std::endl;
                        // No new data, just continue.
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        })
        .detach();
}

void
MainWindow::plot_heatmap()
{
    if(m_results.size() > (unsigned)ui->comboBox_stream->currentIndex())
    {
        // Create a gnuplot instance.
        Gnuplot gp;
        gp << "set terminal wxt persist\n";
        gp << "set pm3d map\n";

        // Example: generate a dummy heatmap.
        // Use the UI dimensions for the matrix.
        int nRows = ui->spinBox_heatmapHeight->value();
        int nCols = ui->spinBox_heatmapWidth->value();
        std::vector<std::vector<double>> matrix(nRows,
                                                std::vector<double>(nCols));
        for(int i = 0; i < nRows; ++i)
            for(int j = 0; j < nCols; ++j)
                // Example function; replace with your own acquired data.
                matrix[i][j] = std::sin(i * 0.1) * std::cos(j * 0.1);

        // Optionally, set colour range using min/max values.
        gp << "set cbrange [" << ui->spinBox_minVal->value() << ":"
           << ui->spinBox_maxVal->value() << "]\n";
        gp << "plot '-' matrix with image title 'Heatmap'\n";
        gp.send2d(matrix);
    }
}

void
MainWindow::scanStream()
{
    //ui->pushButton_scanStream->setEnabled(false);
    m_results = lsl::resolve_streams();

    ui->treeWidget_streams->clear();
    ui->comboBox_stream->clear();
    for(auto &stream : m_results)
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
        subitem_nbChan->setText(1,
                                QString::number(m_results[0].channel_count()));
        //std::vector<QTreeWidgetItem> test(m_results[0].channel_count());

        for(int n = 0; n < m_results[0].channel_count(); n++)
        {
            //std::cout << n << std::endl;
            QTreeWidgetItem *subitem_nbChan_channel =
                new QTreeWidgetItem(subitem_nbChan);
            subitem_nbChan_channel->setText(0, QString::number(n));
            subitem_nbChan_channel->setCheckState(1, Qt::Checked);
        }

        QTreeWidgetItem *subitem_format = new QTreeWidgetItem(item);
        subitem_format->setText(0, "Format");
        subitem_format->setText(
            1, QString::fromStdString(
                   channel_format_str[m_results[0].channel_format()]));

        QTreeWidgetItem *subitem_host = new QTreeWidgetItem(item);
        subitem_host->setText(0, "Host");
        subitem_host->setText(1,
                              QString::fromStdString(m_results[0].hostname()));

        QTreeWidgetItem *subitem_rate = new QTreeWidgetItem(item);
        subitem_rate->setText(0, "Rate");
        subitem_rate->setText(1, QString::number(m_results[0].nominal_srate()));
    }
}

MainWindow::~MainWindow()
{
    std::ofstream myfile("conf.cfg", std::ios::trunc);
    if(myfile.is_open())
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
