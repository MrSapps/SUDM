#include <gmock/gmock.h>

TEST(a, b)
{
    ASSERT_EQ(1, 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    RUN_ALL_TESTS();
    return 0;
}
