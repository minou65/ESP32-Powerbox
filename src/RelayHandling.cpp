// 
// 
// 

#include "common.h"
#include "RelayHandling.h"
#include "webhandling.h"

void setupRelays() {
	Relay* relay_ = &Relay1;
	while (relay_ != nullptr) {
		pinMode(relay_->getGPIO(), OUTPUT);
		relay_ = (Relay*)relay_->getNext();
	}
}

void loopRelays() {
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

void disableAllRelays() {
	Relay* relay_ = &Relay1;
	while (relay_ != nullptr) {
		relay_->setEnabled(false);
		digitalWrite(relay_->getGPIO(), LOW);
		relay_ = (Relay*)relay_->getNext();
	}
}