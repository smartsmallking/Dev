#include "thread_udp.h"
#include "qDebug"

thread_udp::thread_udp()
{

}

void thread_udp::run()
{
    while (true) {
        qDebug()<<"CSimpleThread run!";
        sleep(1);
    }
}
