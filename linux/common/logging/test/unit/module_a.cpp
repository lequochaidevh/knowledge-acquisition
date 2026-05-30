#include "logger.h"

// Define a static logger pointer for this compilation unit (Module A)
static auto logger = LogRegistry::getInstance().getLogger("Module_A");

void runModuleA() {
    logger->setLevel(LogLevel::Trace);  // Module A only handles Debug and above

    LOG_DEBUG("Module A is initializing... code: {}", 101);  // Will print
    LOG_TRACE("This trace will be skipped in Module A");     // Hidden
}