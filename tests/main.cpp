#include <gmock/gmock.h>

TEST(a, b)
{
    ASSERT_EQ(1, 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
