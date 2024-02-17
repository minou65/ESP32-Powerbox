// 
// 
// 

#include "common.h"
#include "RelayHandling.h"
#include "webhandling.h"

void RelaySetup() {
	Relay* _relay = &Relay1;
	while (_relay != nullptr) {
		if (_relay->isActive()) {
			pinMode(_relay->GetGPIO(), OUTPUT);
		}
		_relay = (Relay*)_relay->getNext();
	}
}

void RelayLoop() {
	Relay* _relay = &Relay1;
	while (_relay != nullptr) {
		if (_relay->isActive()) {
			_relay->SetEnabled((gInputPower > _relay->GetPower()));

			if (_relay->IsEnabled()) {
				digitalWrite(_relay->GetGPIO(), HIGH);
			}
			else {
				digitalWrite(_relay->GetGPIO(), LOW);
			}
		}
		_relay = (Relay*)_relay->getNext();
	}
}

void RelayDisableAll() {
	Relay* _relay = &Relay1;
	while (_relay != nullptr) {
		if (_relay->isActive()) {
			_relay->SetEnabled(false);
			digitalWrite(_relay->GetGPIO(), LOW);
		}
		_relay = (Relay*)_relay->getNext();
	}
}