/*
	Thread Wrapper v.2.0

	Copyright (C) 2017 by Sergey A Kryukov
	http://www.SAKryukov.org
	http://www.codeproject.com/Members/SAKryukov	

	Original publications:
	https://www.codeproject.com/Articles/1177478/Thread-Wrapper-CPP
	https://www.codeproject.com/Articles/1177869/Conveyor-Thread-Wrapper-CPP
	https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed
	https://www.codeproject.com/Tips/149540/Simple-Blocking-Queue-for-Thread-Communication-and
*/

#pragma once
#include <iostream>
#include <chrono>
#include "InterlockedProperty.h"
#include "ThreadWrapper.h"

using natural = unsigned long long int;

class MyThread : public ThreadWrapper {
public:
	MyThread() : id(mutex, 2), name(mutex) {}
	InterlockedProperty<int> id;
	InterlockedProperty<const char*> name, help;
	InterlockedProperty<natural> delayMs;
protected:
	void Body() override {
		auto sleep = [=] {
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
		}; //sleep
		int count = 0;
		name = oldName;
		while (true) {
			this->SyncPoint();
			std::cout << count++ << help;
			std::cout << "id: " << id << ", name: " << name << std::endl;
			sleep();
		} //loop
	} //Body
private:
	const char* oldName = "none";
	std::mutex mutex;
}; //class MyThread

class ThreadWrapperDemo {
	enum class command {
		abort = 'a', // abort thread
		quit = 'q', // also abort thread
		sleep = 's',
		wakeUp = 'w',
	};
	static const char* help() { return " a, q: quit, s: sleep, w: wake up, else: change property; "; }
	static bool commandIs(char c, command cmd) { return (int)cmd == (int)c; }
public:
	static void Run(natural delayMs) {
		const char* newName = "new";
		MyThread thread;
		thread.help = help();
		thread.delayMs = delayMs;
		//thread.Suspend(); // can be suspended before start, waken up after
		thread.Start(); // thread.Start(true) for the detached (background)
		char cmd;
		while (true) {
			std::cin >> cmd;
			if (commandIs(cmd, command::abort) || commandIs(cmd, command::quit)) {
				thread.Abort();
				break;
			} else if (commandIs(cmd, command::sleep))
				thread.PutToSleep();
			else if (commandIs(cmd, command::wakeUp))
				thread.WakeUp();
			else {
				thread.id = thread.id + 1; // no ++ defined
				thread.name = newName;
			} //if
		} //loop
		thread.Join();
	} //Run
}; /* class ThreadWrapperDemo */
