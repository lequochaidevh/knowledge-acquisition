#include "logging/logger.h"

// Define a static logger pointer for this compilation unit (Module A)
static auto logger = LogRegistry::getInstance().getLogger("Qrb5165GstreamerInterface");

void runModuleA() {
    logger->setLevel(LogLevel::Trace);  // Module A only handles Debug and above

    HARIS_LOG_DEBUG("Module A is initializing... code: {}", 101);    // Will print
    HARIS_LOG_TRACE("This trace will be showed in Module A");        //
    HARIS_LOG_INFO("This INFO will be showed in Module A");          //
    HARIS_LOG_CRITICAL("This CRITICAL will be showed in Module A");  //
}
