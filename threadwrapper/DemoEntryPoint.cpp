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

#include "stdafx.h"
#include "ThreadWrapperDemo.h"
#include "ConveyorThreadWrapperDemo.h"

int main() {
	auto delay = 200;
	ThreadWrapperDemo::Run(delay);
	std::cout << std::endl << std::endl
		<< "ThreadWrapper demo complete. Now, ConveyorThreadWrapper demo:" << std::endl
		<< "Enter any character: ";
	ConveyorThreadWrapperDemo::Run(delay);
	return 0;
} /* main */
