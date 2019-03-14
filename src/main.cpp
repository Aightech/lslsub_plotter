#include "include/mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


    // resolve the stream of interest & make an inlet to get data from the first result
   // std::vector<lsl::stream_info> results = lsl::resolve_stream("name", argc > 1 ? argv[1] : "SimpleStream");
    //lsl::stream_inlet inlet(results[0]);

    // receive data & time stamps forever (not displaying them here)
    /*std::vector<float> sample;
    while (true)
    {
        inlet.pull_sample(sample);
        for(int i = 0 ; i< sample.size(); i++)
            std::cout << sample[i] << std::endl;
        std::cout << std::endl;
    }*/

    return a.exec();
}
