#include "logging/logger.h"

static auto logger = LogRegistry::get_instance().get_logger("Module_B");

void runModuleB() {
    logger->set_level(LogLevel::Warn);  // Module B only handles Warn, Error and Critical

    HARIS_LOG_WARN("Warning in Module B");                                 // Hidden because level is set to Error
    HARIS_LOG_ERROR("Critical connection lost on interface: {}", "eth0");  // Will print
}
