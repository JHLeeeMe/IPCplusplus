/// sysv_mq_test.cpp
///

#include "gtest/gtest.h"

#include "sysv/sysv_mq.h"

TEST(sysv_mq, utils)
{ // IPCplusplus::sysv::utils
    {  // create_key()
        ASSERT_EQ(-1, sysv::utils::create_key("./", 0));
        ASSERT_EQ(-1, sysv::utils::create_key("./", 256));
        ASSERT_NE(-1, sysv::utils::create_key("./", -1));

        ASSERT_EQ(::ftok("./", 255), sysv::utils::create_key("./", 255));
        ASSERT_EQ(::ftok("./", 1), sysv::utils::create_key("./", 257));
    }
}

TEST(sysv_mq, mq)
{  // IPCplusplus::sysv::mq
    {  // ePermission
        using ePermission_Ty = sysv::mq::ePermission;

        ASSERT_EQ(0666, static_cast<uint32_t>(ePermission_Ty::RWRWRW));
        ASSERT_EQ(0422, static_cast<uint32_t>(ePermission_Ty::R_____ |
                                              ePermission_Ty::___W__ |
                                              ePermission_Ty::_____W));
        ASSERT_EQ(ePermission_Ty::RWRWRW, ePermission_Ty::RW____ |
                                          ePermission_Ty::__RW__ |
                                          ePermission_Ty::____RW);
    }

    {  // class MQueue
        using ePermission_Ty = sysv::mq::ePermission;
        using MQueue_Ty      = sysv::mq::MQueue;

        const key_t    key = sysv::utils::create_key("./", 255);
        ePermission_Ty flag{ ePermission_Ty::RWR_R_ };
        MQueue_Ty mqueue{ key, flag };

        {  // change_permision()
            ASSERT_EQ(0644, static_cast<uint16_t>(mqueue.get_permission()));

            flag = ePermission_Ty::_W____ |
                   ePermission_Ty::___W__ |
                   ePermission_Ty::____R_;
            mqueue.change_permission(flag);

            ASSERT_EQ(0224, static_cast<uint16_t>(mqueue.get_permission()));
        }

        {  // get_queue_info()
            struct msqid_ds info{ mqueue.get_queue_info() };
            std::cout << "info.msg_ctime: "     << info.msg_ctime     << '\n'
                      << "info.msg_lrpid: "     << info.msg_lrpid     << '\n'
                      << "info.msg_lspid: "     << info.msg_lspid     << '\n'
                      << "info.msg_qbytes: "    << info.msg_qbytes    << '\n'
                      << "info.msg_qnum: "      << info.msg_qnum      << '\n'
                      << "info.msg_rtime: "     << info.msg_rtime     << '\n'
                      << "info.msg_stime: "     << info.msg_stime     << '\n'
                      << "info.msg_perm.uid: "  << info.msg_perm.uid  << '\n'
                      << "info.msg_perm.cuid: " << info.msg_perm.cuid << '\n'
                      << "info.msg_perm.gid: "  << info.msg_perm.gid  << '\n'
                      << "info.msg_perm.cgid: " << info.msg_perm.cgid << '\n'
                      << "info.msg_perm.mode: " << std::oct << info.msg_perm.mode
                      << std::endl;

            flag = (ePermission_Ty::RW____ |
                    ePermission_Ty::__RW__ |
                    ePermission_Ty::____RW);
            mqueue.change_permission(flag);
            ASSERT_EQ(static_cast<uint16_t>(flag),
                      mqueue.get_queue_info().msg_perm.mode);
        }
    }
}