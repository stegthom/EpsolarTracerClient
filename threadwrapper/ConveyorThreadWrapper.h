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
#include "ThreadWrapper.h"
#include "Delegate.h"
#include <queue>

template<typename PARAM>
class ConveyorThreadWrapper : public ThreadWrapper {
public:

	ConveyorThreadWrapper() {
		ThreadWrapper::InitializeSyncPointDelegate(syncDelegate, this);
	} //ConveyorThreadWrapper

	~ConveyorThreadWrapper() {
		std::lock_guard<std::mutex> lock(queueMutex);
		while (!queue.empty()) {
			auto element = queue.front();
			queue.pop();
			delete element;
		} //loop
	}; //~ConveyorThreadWrapper
	
	using SyncDelegate = SA::delegate<void(bool)>;
	using TaskDelegate = SA::delegate<void(SyncDelegate&, PARAM)>;

	void Enqueue(TaskDelegate& task, PARAM argument) {
		std::lock_guard<std::mutex> lock(queueMutex);
		queue.push(new QueueItem(task, argument));
		queueCondition.notify_one();
	} //Enqueue

	enum class AbortDepth { currentTask, entierThread, };
	void Abort(AbortDepth depth = AbortDepth::currentTask) {
		SetAbort(true, depth == AbortDepth::currentTask);
	} //Abort

	SA::delegate<void(const PARAM&)> CurrentTaskAborted;

protected:

	virtual void OnCurrentTaskAborted(const PARAM& current) {
		if (!CurrentTaskAborted.isNull())
			CurrentTaskAborted(current);
	} //OnCurrentTaskAborted

protected:

	void Body() override final {
		while (true) {
			QueueItem task;
			getTask(task);
			try {
				(*task.delegate)(syncDelegate, task.parameter);
			} catch (ShallowThreadAbortException&) {
				SetAbort(false);
				OnCurrentTaskAborted(task.parameter);
			} //exception
		} //loop
	} //Body

private:
	
	SyncDelegate syncDelegate;

	struct QueueItem {
		QueueItem() = default;
		QueueItem(TaskDelegate& del, PARAM param) {
			delegate = &del;
			parameter = param;
		} //QueueItem
		TaskDelegate* delegate;
		PARAM parameter;
	}; //struct QueueItem

	std::queue<QueueItem*> queue; // protected by queueState and queueMutex
	std::mutex queueMutex;
	std::condition_variable queueCondition;

	void getTask(QueueItem& itemCopy) {
		std::unique_lock<std::mutex> ul(queueMutex);
		queueCondition.wait(ul, [=] {
			return queue.size() > 0;
		});
		auto front = queue.front();
		itemCopy.delegate = front->delegate;
		itemCopy.parameter = front->parameter;
		queue.pop();
		delete front;
	} //getTask

}; /* class ConveyorThreadWrapper */
