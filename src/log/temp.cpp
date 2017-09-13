#include "temp.h"
#include "log/registry.h"

TemporaryLogLevel::TemporaryLogLevel(const std::string &name, int level,
    bool cond)
    : name(name), previous(GroupRegistry::getInstance()->getSetting(name)) {

    if(cond) {
        GroupRegistry::getInstance()->applySetting(name, level);
    }
}

TemporaryLogLevel::~TemporaryLogLevel() {
    GroupRegistry::getInstance()->applySetting(name, previous);
}
