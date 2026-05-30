#include "logger.h"

static auto logger = LogRegistry::getInstance().getLogger("Module_B");

void runModuleB() {
    logger->setLevel(LogLevel::Error);  // Module B only handles Error and Critical

    LOG_WARN("Warning in Module B");                                 // Hidden because level is set to Error
    LOG_ERROR("Critical connection lost on interface: {}", "eth0");  // Will print
}