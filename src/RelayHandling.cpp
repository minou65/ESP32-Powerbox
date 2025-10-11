// 
// 
// 

#include "common.h"
#include "RelayHandling.h"
#include "webhandling.h"

void RelaySetup() {
	Relay* relay_ = &Relay1;
	while (relay_ != nullptr) {
		pinMode(relay_->getGPIO(), OUTPUT);
		relay_ = (Relay*)relay_->getNext();
	}
}

void RelayLoop() {
	Relay* relay_ = &Relay1;
	while (relay_ != nullptr) {
		relay_->setEnabled((gInputPower > relay_->getPower()));

		if (relay_->isEnabled()) {
			digitalWrite(relay_->getGPIO(), HIGH);
		}
		else {
			digitalWrite(relay_->getGPIO(), LOW);
			}

		relay_ = (Relay*)relay_->getNext();
	}
}

void RelayDisableAll() {
	Relay* relay_ = &Relay1;
	while (relay_ != nullptr) {
		relay_->setEnabled(false);
		digitalWrite(relay_->getGPIO(), LOW);
		relay_ = (Relay*)relay_->getNext();
	}
}