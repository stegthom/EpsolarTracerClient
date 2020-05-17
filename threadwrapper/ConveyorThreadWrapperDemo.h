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
#include "ConveyorThreadWrapper.h"

class IntegerConveyor : public ConveyorThreadWrapper<int> {
protected:
	virtual void OnCurrentTaskAborted(const int& current) override {
		std::cout << "Current task aborted: " << current;
		ConveyorThreadWrapper<int>::OnCurrentTaskAborted(current);
	} //OnCurrentTaskAborted
}; //class IntegerConveyor

class ConveyorThreadWrapperDemo : public ConveyorThreadWrapper<int> {
	enum class command {
		quit = 'q', // entire thread
		abort = 'a', // current task
		sleep = 's',
		wakeUp = 'w',
	};
	static const char* help() { return "a: task abort, q: quit, s: sleep, w: wake up, key: "; }
	static bool commandIs(char c, command cmd) { return (int)cmd == (int)c; }
public:
	using natural = unsigned long long int;
	static void Run(natural delayMs) {
		natural max = 100;
		IntegerConveyor cv;
		cv.CurrentTaskAborted = [](int) -> void {
			std::cout << ". Enter any character: ";
		}; //cv.CurrentTaskAborted
		using cType = decltype(cv);
		auto sleep = [delayMs] {
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
		}; //sleep
		auto Hailstone = [] (natural a) -> natural {
			if (a % 2 == 0)
				return a / 2;
			else
				return 3 * a + 1;
		}; //Hailstone (iteration)
		auto HailstoneStoppingTime = [Hailstone] (natural n) -> natural {
			natural stoppingTime = 0;
			while (n != 1) {
				n = Hailstone(n);
				++stoppingTime;
			} //loop
			return stoppingTime;
		}; //HailstoneStoppingTime
		auto lambdaHailstone = [HailstoneStoppingTime, sleep, max] (cType::SyncDelegate& sync, int value) {
			for (natural count = 1; count < max; ++count) {
				natural stoppingTime = HailstoneStoppingTime(count + value);
				sync(false);
				std::cout
					<< count << ": " << help()
					<< value << " => Hailstone " << stoppingTime << std::endl;
				sleep();
			} //loop
		}; //lambdaHailstone
		natural mod = 1;
		mod = mod << 32; // NOT 1 < 32 !!!
		natural multiplier = 1664525; // Numerical Recipes
		natural increment = 1013904223; // Numerical Recipes
		auto Lgc = [mod, multiplier, increment](natural n) -> natural {
			return (n * multiplier + increment) % mod;
		}; //Lgc
		auto lambdaLgc = [Lgc, sleep, max](cType::SyncDelegate& sync, int value) {
			natural n = value;
			for (natural count = 0; count < max; count++) {
				n = Lgc(n);
				sync(false);
				std::cout
					<< help()
					<< value << " => LGC " << (n) << std::endl;
				sleep();
			} //loop
		}; //lambdaLgc
		SA::delegate<void(cType::SyncDelegate&, int)> delHailstone = lambdaHailstone;
		SA::delegate<void(cType::SyncDelegate&, int)> delLgc = lambdaLgc;
		cv.Start();
		char cmd;
		while (true) {
			std::cin >> cmd;
			if (commandIs(cmd, command::abort))
				cv.Abort();
			else if (commandIs(cmd, command::quit)) {
				cv.Abort(cType::AbortDepth::entierThread);
				break;
			} else if (commandIs(cmd, command::sleep))
				cv.PutToSleep();
			else if (commandIs(cmd, command::wakeUp))
				cv.WakeUp();
			else {
				int cmdValue = (int)cmd;
				if (cmdValue % 2 == 0)
					cv.Enqueue(delHailstone, cmdValue);
				else
					cv.Enqueue(delLgc, cmdValue);
			} //if
		} //loop
		cv.Join();
	} //Run
}; /* class ConveyorThreadWrapperDemo */
