#include "logging/logger.h"

static auto logger = LogRegistry::getInstance().getLogger("Module_B");

void runModuleB() {
    logger->setLevel(LogLevel::Warn);  // Module B only handles Warn, Error and Critical

    HARIS_LOG_WARN("Warning in Module B");                                 // Hidden because level is set to Error
    HARIS_LOG_ERROR("Critical connection lost on interface: {}", "eth0");  // Will print
}
