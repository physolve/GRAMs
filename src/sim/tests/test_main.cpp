#include <gtest/gtest.h>

#include <QCoreApplication>

// QCoreApplication нужен машине состояний, таймерам и сетевым тестам.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
