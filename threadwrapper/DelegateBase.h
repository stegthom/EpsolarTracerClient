/*
	Thread Wrapper v.2.0
	Delegates

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

namespace SA {
	
	template<typename T>
	class delegate_base;
	
	template<typename RET, typename ...PARAMS>
	class delegate_base<RET(PARAMS...)> {

	protected:

		using stub_type = RET(*)(void* this_ptr, PARAMS...);

		struct InvocationElement {
			InvocationElement() = default;
			InvocationElement(void* this_ptr, stub_type aStub) : object(this_ptr), stub(aStub) {}
			void Clone(InvocationElement& target) const {
				target.stub = stub;
				target.object = object;
			} //Clone
			bool operator ==(const InvocationElement& another) const {
				return another.stub == stub && another.object == object;
			} //==
			bool operator !=(const InvocationElement& another) const {
				return another.stub != stub || another.object != object;
			} //!=
			void* object = nullptr;
			stub_type stub = nullptr;
		}; //InvocationElement

	}; //class delegate_base

} /* namespace SA */
