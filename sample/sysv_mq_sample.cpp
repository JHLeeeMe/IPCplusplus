/// sysv_mq_sample.cpp
///

#include <iostream>

#include "ipcplusplus.h"

int main()
{
    namespace utils = sysv::utils;
    namespace mq    = sysv::mq;

    const key_t     key   = utils::create_key("./", 255);
    mq::ePermission flag  { mq::ePermission::RWRWRW };
    mq::MQueue      mqueue{ key, flag };
    if (mqueue.err() != 0)
    {
        std::cerr << "errno: " << mqueue.err() << std::endl;
        return 1;
    }

    flag = (mq::ePermission::RW____ |
            mq::ePermission::__R___ |
            mq::ePermission::____R_);
    mqueue.change_permission(flag);

    const long mtype = 21;
    for (size_t i = 0; i < 10; i++)
    {
        //if (mqueue.send(std::to_string(i), mtype) < 0)
        if (mqueue.send_nowait(std::to_string(i), mtype) < 0)
        {
            std::cerr << mqueue.err() << std::endl;
        }
    }

    for (size_t i = 0; i < 10; i++)
    {
        if (mqueue.receive(mtype) < 0)
        {
            std::cerr << mqueue.err() << std::endl;
        }

        std::cout << mqueue.msg() << '\n';
    }

    if (mqueue.receive_nowait(mtype) < 0)
    {
        std::cerr << "errno: " << mqueue.err() << std::endl;
    }

    return 0;
}