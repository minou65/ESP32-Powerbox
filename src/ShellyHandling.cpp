#include "ShellyHandling.h"

#include "common.h"
#include "webhandling.h"

void setupShellys() {
};

void disableAllShellys() {
    Shelly* shelly_ = &Shelly1;
    while (shelly_ != nullptr) {
        if (shelly_->isActive()) {
            shelly_->setEnabled(false);
        }
        shelly_ = (Shelly*)shelly_->getNext();
    }
}