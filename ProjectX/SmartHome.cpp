#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <stack>
#include "device.h"
using namespace std;

class Command {
public:
	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual ~Command() {}
};

class TurnOnCommand : public Command {
	shared_ptr<Device> device;

public :
	TurnOnCommand(shared_ptr<Device> dev) : device(dev) {}
	void execute() override{
		device->turnOn();
	}
	void undo() override{
		device->turnOff();
	}
};

class TurnOffCommand : public Command {
	shared_ptr<Device> device;

public:
	TurnOffCommand(shared_ptr<Device> dev) : device(dev) {}
	void execute() override {
		device->turnOff();
	}
	void undo() override {
		device->turnOn();
	}
};

class Remote {
	stack<shared_ptr<Command>> activity;
public:
	void executeCmd(shared_ptr<Command> cmd) {
		cmd->execute();
		activity.push(cmd);
	}

	void undoCmd() {
		if (!activity.empty()) {
			activity.top()->undo();
			activity.pop();
		}
	}
};


class DeviceFactory {
public:
	static shared_ptr<Device> createDevice(const string& type) {
		if (type == "Light") return make_shared<Light>();
		if (type == "Fan") return make_shared<Fan>();
		return nullptr;
	}
};

class Observer {
public:
	virtual void update(const string& event) = 0;

};


class Subject {
	vector<Observer*> observers;
public:
	void addObserver(Observer* observer) {
		observers.push_back(observer);
	}
	void notifyAllObservers(const string& event) {
		for (Observer* observer : observers) {
			observer->update(event);
		}
	}
};

class DoorSensor : public Subject {
public :
	void detectIntrusion() {
		notifyAllObservers("Intruders!!! \n");
	}
};

class Mobile : public Observer {
public:
	void update(const string& event) override {
		cout << "Mobile notification: " << event << endl;

	}
};


class ScheduleStrategy {
public:
	virtual void executeSchedule(map<string, shared_ptr<Device>>& devices) = 0;
};

class NightTimeSchedule : public ScheduleStrategy {
public:
	void executeSchedule(map<string, shared_ptr<Device>>& devices) override {
		for (auto it = devices.begin(); it != devices.end(); ++it) {
			const string& name = it->first;
			shared_ptr<Device>& dev = it->second;
			if (name.find("Light") != string::npos) {
				dev->turnOff();
			}
		}

	}
};

class Scheduler {
	shared_ptr<ScheduleStrategy> strategy;
public:
	void setStrategy(shared_ptr<ScheduleStrategy> strat) { strategy = strat; }
	void run(map<string, shared_ptr<Device>>& devices) {
		if (strategy) strategy->executeSchedule(devices);
	}
};
class CentralController {
	map<string, shared_ptr<Device>> devices;
	CentralController() {}
public:
	static CentralController& getInstance() {
		static CentralController instance;
		return instance;
	}

	void registerDevice(const string& name, shared_ptr<Device> dev) {
		devices[name] = dev;
	}

	shared_ptr<Device> getDevice(const string& name) {
		return devices.count(name) ? devices[name] : nullptr;
	}

	map<string, shared_ptr<Device>>& getAllDevices() {
		return devices;
	}
};



int main() {
	auto& controller = CentralController::getInstance();

	auto light = DeviceFactory::createDevice("Light");
	controller.registerDevice("LivingRoomLight", light);


	Scheduler scheduler;
	scheduler.setStrategy(make_shared<NightTimeSchedule>());
	scheduler.run(controller.getAllDevices());
	cout << light->getStatus() << endl;

	Remote remote;
	auto cmd = make_shared<TurnOnCommand>(light);
	remote.executeCmd(cmd);
	cout << light->getStatus() << endl;


	DoorSensor sensor;
	Mobile app;
	sensor.addObserver(&app);
	sensor.detectIntrusion();

	return 0;
}
