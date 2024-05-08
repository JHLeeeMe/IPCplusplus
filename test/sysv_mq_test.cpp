/// ./test/sysv_mq_test.cpp
///

#include "gtest/gtest.h"

#include "sysv/sysv_mq.h"

TEST(SystemV, sysv_mq)
{
    testing::internal::CaptureStdout();

    ::IPCplusplus::sysv::mq::MQueue a{};
                   sysv::mq::MQueue b{};
                         mq::MQueue c{};

    std::string output{
        testing::internal::GetCapturedStdout() };
    
    ASSERT_EQ(output,
              "Hello, world!\nHello, world!\nHello, world!\n");
}