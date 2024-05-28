/// sysv_mq_test.cpp
///

#include <chrono>
#include <thread>

#include "gtest/gtest.h"

//#include "ipcplusplus.h"
#include "ipcplusplus/sysv/sysv_mq.h"

TEST(utils, create_key)
{ // ::ipcplusplus::sysv::utils
    ASSERT_EQ(-1, sysv::utils::create_key("./", 0));
    ASSERT_EQ(-1, sysv::utils::create_key("./", 256));
    ASSERT_NE(-1, sysv::utils::create_key("./", -1));

    ASSERT_EQ(::ftok("./", 255), sysv::utils::create_key("./", 255));
    ASSERT_EQ(::ftok("./", 1), sysv::utils::create_key("./", 257));
}

TEST(mq, ePermission)
{  // ::ipcplusplus::sysv::mq
    using ePermission_Ty = sysv::mq::ePermission;

    ASSERT_EQ(0666, static_cast<uint32_t>(ePermission_Ty::RWRWRW));
    ASSERT_EQ(0422, static_cast<uint32_t>(ePermission_Ty::R_____ |
                                          ePermission_Ty::___W__ |
                                          ePermission_Ty::_____W));
    ASSERT_EQ(ePermission_Ty::RWRWRW, ePermission_Ty::RW____ |
                                      ePermission_Ty::__RW__ |
                                      ePermission_Ty::____RW);
}

TEST(mq, MQueue)
{  // ::ipcplusplus::sysv::mq
    using ePermission_Ty = sysv::mq::ePermission;
    using MQueue_Ty      = sysv::mq::MQueue;

    const key_t    key   = sysv::utils::create_key("./", 255);
    ePermission_Ty flag  { ePermission_Ty::RWR_R_ };
    MQueue_Ty      mqueue{ key, flag };
    if (mqueue.err() != 0)
    {
        std::cout << "mqueue.err() != 0" << std::endl;
        return;
    }

    ASSERT_NO_THROW(MQueue_Ty mq(key, flag));

    {  // Queue Already exists. (errno == EEXIST)
        const key_t    key_1   = sysv::utils::create_key("./", 1);

        ePermission_Ty flag_1  { ePermission_Ty::RWRWRW };
        MQueue_Ty      mqueue_1{ key_1, flag_1 };
        if (mqueue_1.err() != 0)
        {
            std::cout << "mqueue_1.err() != 0" << std::endl;
            return;
        }

        ePermission_Ty flag_2  { ePermission_Ty::RWR_R_ };
        MQueue_Ty      mqueue_2{ key_1, flag_2, 1000 };
        if (mqueue_2.err() != 0)
        {
            std::cout << "mqueue_2.err() != 0" << std::endl;
            return;
        }

        ASSERT_EQ(
            static_cast<uint16_t>(mqueue_1.permission()),
            static_cast<uint16_t>(mqueue_2.permission())
        );
        ASSERT_EQ(
            mqueue_1.queue_info().msg_ctime,
            mqueue_2.queue_info().msg_ctime
        );
        ASSERT_EQ(
            mqueue_1.queue_info().msg_perm.mode,
            mqueue_2.queue_info().msg_perm.mode
        );
    }

    {  // change_permision()
        ASSERT_EQ(0644, static_cast<uint16_t>(mqueue.permission()));

        flag = ePermission_Ty::_W____ |
               ePermission_Ty::___W__ |
               ePermission_Ty::____R_;
        mqueue.change_permission(flag);

        ASSERT_EQ(0224, static_cast<uint16_t>(mqueue.permission()));
    }

    {  // queue_info()
        struct msqid_ds info{ mqueue.queue_info() };
        ASSERT_EQ(typeid(struct msqid_ds), typeid(info));

        flag = (ePermission_Ty::RW____ |
                ePermission_Ty::__RW__ |
                ePermission_Ty::____RW);
        mqueue.change_permission(flag);
        struct msqid_ds info_1{ mqueue.queue_info() };
        ASSERT_EQ(static_cast<uint16_t>(flag), info_1.msg_perm.mode);
    }

    const long mtype = 21;

    {  // send()
        for (size_t i = 0; i < 10; i++)
        {
            //if (mqueue.send(std::to_string(i), mtype) < 0)
            if (mqueue.send_nowait(std::to_string(i), mtype) < 0)
            {
                std::cout << mqueue.err() << std::endl;
            }
        }
    }

    {  // receive()
        testing::internal::CaptureStdout();

        std::string str{};
        for (size_t i = 0; i < 10; i++)
        {
            if (mqueue.receive(mtype) < 0)
            {
                std::cout << mqueue.err() << std::endl;
            }

            const std::string msg{ mqueue.msg() };
            std::cout << msg << '\n';
            str += (msg + '\n');
        }

        std::string output{ testing::internal::GetCapturedStdout() };
        ASSERT_EQ(str, output);
    }

    {  // receive_nowait()
        if (mqueue.receive_nowait(mtype) < 0)
        {
            ASSERT_EQ(ENOMSG, mqueue.err());
        }
    }
}
