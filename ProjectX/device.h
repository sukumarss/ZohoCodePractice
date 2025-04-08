
#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
using std::string;

class Device {
public:
	virtual void turnOn() = 0;
	virtual void turnOff() = 0;
	virtual string getStatus() = 0;
	virtual ~Device() {}
};

class Light : public Device {
private:
	bool status = false;
public:
	void turnOn() override;
	void turnOff() override;
	string getStatus() override;

};

class Fan : public Device {
private:
	bool status = false;
public:
	void turnOn() override;
	void turnOff() override;
	string getStatus() override;

};

class AirConditioner : Device {
private:
	bool status = false;
public:
	void turnOn() override;
	void turnOff() override;
	string getStatus() override;

};

#endif