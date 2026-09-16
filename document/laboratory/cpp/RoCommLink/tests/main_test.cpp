
#include "test1_udp_fork.h"
#include "test2_serializer.h"
#include "test3_ack_check.h"

using namespace std::chrono_literals;

int main() {
    // SerializerTest::main();
    // UDP_InterfaceTest::main();
    CommandBlockingTest::main();
    return 0;
}