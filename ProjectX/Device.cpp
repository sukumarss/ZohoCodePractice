#include "device.h"

void Light::turnOn() {
	status = true;
}
void Light::turnOff() {
	status = false;
}
string Light::getStatus() {
	if (status) {
		return "Light is On";
	}
	return "Light is Off";
}

void Fan::turnOn() {
	status = true;
}
void Fan::turnOff() {
	status = false;
}
string Fan::getStatus() {
	if (status) {
		return "Light is On";
	}
	return "Light is Off";
}

void AirConditioner::turnOn() {
	status = true;
}
void AirConditioner::turnOff() {
	status = false;
}
string AirConditioner::getStatus() {
	if (status) {
		return "Light is On";
	}
	return "Light is Off";
}

