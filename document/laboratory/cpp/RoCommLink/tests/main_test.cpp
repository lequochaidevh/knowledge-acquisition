
#include "test1_udp_fork.h"
#include "test2_serializer.h"

using namespace std::chrono_literals;

int main() {
    SerializerTest::main();
    UDP_InterfaceTest::main();
    return 0;
}