
#include "test1_udp_fork.h"
#include "test2_serializer.h"
#include "test3_ack_check.h"
#include "test4_routing.h"

using namespace std::chrono_literals;

int main() {
    // SerializerTest::main();
    // UDP_InterfaceTest::main();
    // CommandBlockingTest::main();
    RoutingTest::main();
    return 0;
}