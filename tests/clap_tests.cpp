#include <gtest/gtest.h>

#include "clap/clap.h"

TEST(clap, simple)
{
    ASSERT_EQ(clap::simple(), 42);
}
