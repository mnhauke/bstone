/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/


//
// Multi-threaded task manager.
//

#include "bstone_mt_task_mgr.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "bstone_exception.h"
#include "bstone_spinlock.h"


namespace bstone
{

namespace detail
{

// ==========================================================================
// MtTaskQueue
//

class MtTaskQueue
{
public:
	explicit MtTaskQueue(
		int size);

	MtTaskQueue(
		const MtTaskQueue& that) = delete;

	MtTaskQueue& operator=(
		const MtTaskQueue& that) = delete;


	void push(
		MtTaskPtr mt_task);

	void push(
		MtTaskPtr* mt_tasks,
		int mt_task_count);

	bool pop(
		MtTaskPtr& mt_task) noexcept;


private:
	using Items = std::vector<MtTaskPtr>;
	using Index = unsigned int;
	using MtIndex = std::atomic<Index>;
	using MtSpinFlagLock = std::unique_lock<Spinlock>;

	Index size_;
	MtIndex mt_read_index_;
	MtIndex mt_write_index_;
	Spinlock mt_spin_flag_;
	Items items_;


	[[noreturn]]
	static void fail(
		const char* message);

	[[noreturn]]
	static void fail_nested(
		const char* message);
}; // MtTaskQueue

//
// MtTaskQueue
// ==========================================================================


} // detail


// ==========================================================================
// MtTaskMgrImplException
//

class MtTaskMgrImplException :
	public Exception
{
public:
	explicit MtTaskMgrImplException(
		const char* message) noexcept
		:
		Exception{"MT_TASK_MGR", message}
	{
	}
}; // MtTaskMgrImplException

//
// MtTaskMgrImplException
// ==========================================================================


// ==========================================================================
// MtTaskMgrImpl
//

class MtTaskMgrImpl final :
	public MtTaskMgr
{
public:
	MtTaskMgrImpl(
		int thread_reserve_count,
		int max_task_count);

	~MtTaskMgrImpl() override;


	int get_max_threads() const noexcept override;

	int get_thread_count() const noexcept override;


	void add_tasks(
		MtTaskPtr* mt_tasks,
		int mt_task_count) override;

	void add_tasks_and_wait_for_added(
		MtTaskPtr* mt_tasks,
		int mt_task_count) override;


private:
	struct MtThread
	{
		bool is_failed_;
		std::exception_ptr exception_;

		std::thread thread_;
	}; // MtThread

	using MtThreadPtr = MtThread*;

	using MtThreads = std::vector<MtThread>;


	int max_threads_;
	int thread_count_;
	int concurrency_reserve_;

	std::atomic_bool mt_is_quit_;

	detail::MtTaskQueue mt_task_queue_;
	MtThreads mt_threads_;


	[[noreturn]]
	static void fail(
		const char* message);

	[[noreturn]]
	static void fail_nested(
		const char* message);


	void initialize_concurrency();

	void initialize_threads();

	void initialize();

	void uninitialize();

	bool mt_is_quit();

	//
	// Returns:
	//    - "true" if task was picked and executed.
	//    - "false" if there are no more tasks.
	//
	bool try_pick_and_execute();

	void mt_thread_func(
		MtThread* mt_thread);
}; // MtTaskMgr

using MtTaskMgrImplPtr = MtTaskMgrImpl*;
using MtTaskMgrImplUPtr = std::unique_ptr<MtTaskMgrImpl>;

//
// MtTaskMgrImpl
// ==========================================================================


namespace detail
{


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class MtSpinFlagException :
	public Exception
{
public:
	explicit MtSpinFlagException(
		const char* message)
		:
		Exception{"MT_SPIN_FLAG", message}
	{
	}
}; // MtSpinFlagException

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class MtTaskQueueException :
	public Exception
{
public:
	explicit MtTaskQueueException(
		const char* message)
		:
		Exception{"MT_TASK_QUEUE", message}
	{
	}
}; // MtTaskQueueException

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// ==========================================================================
// MtTaskQueue
//

MtTaskQueue::MtTaskQueue(
	int size)
	:
	size_{},
	mt_read_index_{},
	mt_write_index_{},
	mt_spin_flag_{},
	items_{}
{
	if (size <= 0)
	{
		fail("Max size out of range.");
	}

	size_ = static_cast<Index>(size);
	items_.resize(size_);
}

void MtTaskQueue::push(
	MtTaskPtr mt_task)
{
	if (!mt_task)
	{
		fail("Null task.");
	}

	MtSpinFlagLock flag_lock{mt_spin_flag_};

	const auto read_index = mt_read_index_.load(std::memory_order_acquire);
	const auto write_index = mt_write_index_.load(std::memory_order_acquire);

	const auto new_write_index = (write_index + 1) % size_;

	if (read_index == new_write_index)
	{
		fail("Queue overflow.");
	}

	items_[write_index] = mt_task;

	mt_write_index_.store(new_write_index, std::memory_order_release);
}

void MtTaskQueue::push(
	MtTaskPtr* mt_tasks,
	int mt_task_count)
{
	if (!mt_tasks)
	{
		fail("Null task list.");
	}

	if (mt_task_count <= 0)
	{
		fail("Task count out of range.");
	}

	const auto has_null = std::any_of(
		mt_tasks,
		mt_tasks + mt_task_count,
		[](const auto item)
		{
			return !item;
		}
	);

	if (has_null)
	{
		fail("Null task.");
	}

	MtSpinFlagLock flag_lock{mt_spin_flag_};

	const auto read_index = mt_read_index_.load(std::memory_order_acquire);

	auto write_index = mt_write_index_.load(std::memory_order_acquire);
	auto new_write_index = write_index;

	for (int i = 0; i < mt_task_count; ++i)
	{
		new_write_index += 1;
		new_write_index %= size_;

		if (read_index == new_write_index)
		{
			fail("Queue overflow.");
		}

		items_[write_index] = mt_tasks[i];

		write_index += 1;
		write_index %= size_;
	}

	mt_write_index_.store(new_write_index, std::memory_order_release);
}

bool MtTaskQueue::pop(
	MtTaskPtr& mt_task) noexcept
{
	MtSpinFlagLock flag_lock{mt_spin_flag_};

	const auto read_index = mt_read_index_.load(std::memory_order_acquire);
	const auto write_index = mt_write_index_.load(std::memory_order_acquire);

	if (read_index == write_index)
	{
		return false;
	}

	const auto new_read_index = (read_index + 1) % size_;

	mt_task = items_[read_index];

	mt_read_index_.store(new_read_index, std::memory_order_release);

	return true;
}

[[noreturn]]
void MtTaskQueue::fail(
	const char* message)
{
	throw MtTaskQueueException{message};
}

[[noreturn]]
void MtTaskQueue::fail_nested(
	const char* message)
{
	std::throw_with_nested(MtTaskQueueException{message});
}

//
// MtTaskQueue
// ==========================================================================


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class MtTaskMgrException :
	public Exception
{
public:
	explicit MtTaskMgrException(
		const char* message)
		:
		Exception{"MT_TASK_MGR", message}
	{
	}
}; // MtTaskMgrException

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


} // detail


// ==========================================================================
// MtTaskMgrImpl
//

MtTaskMgrImpl::MtTaskMgrImpl(
	int concurrency_reserve,
	int max_task_count)
	:
	max_threads_{},
	thread_count_{},
	concurrency_reserve_{concurrency_reserve},
	mt_is_quit_{},
	mt_task_queue_{max_task_count},
	mt_threads_{}
{
	initialize();
}

MtTaskMgrImpl::~MtTaskMgrImpl()
{
	uninitialize();
}

int MtTaskMgrImpl::get_max_threads() const noexcept
{
	return max_threads_;
}

int MtTaskMgrImpl::get_thread_count() const noexcept
{
	return thread_count_;
}

void MtTaskMgrImpl::add_tasks(
	MtTaskPtr* mt_tasks,
	int mt_task_count)
{
	mt_task_queue_.push(mt_tasks, mt_task_count);
}

void MtTaskMgrImpl::add_tasks_and_wait_for_added(
	MtTaskPtr* mt_tasks,
	int mt_task_count)
{
	add_tasks(mt_tasks, mt_task_count);

	auto is_quit = false;
	auto is_completed = false;

	while (!is_quit && !is_completed)
	{
		try_pick_and_execute();

		is_completed = std::all_of(
			mt_tasks,
			mt_tasks + mt_task_count,
			[](const auto item)
			{
				return item->is_completed();
			}
		);

		is_quit = mt_is_quit();
	}

	for (auto& mt_thread : mt_threads_)
	{
		if (mt_thread.is_failed_)
		{
			std::rethrow_exception(mt_thread.exception_);
		}
	}

	for (int i = 0; i < mt_task_count; ++i)
	{
		const auto& mt_task = mt_tasks[i];

		if (mt_task->is_failed())
		{
			std::rethrow_exception(mt_task->get_exception_ptr());
		}
	}
}

[[noreturn]]
void MtTaskMgrImpl::fail(
	const char* message)
{
	throw MtTaskMgrImplException{message};
}

[[noreturn]]
void MtTaskMgrImpl::fail_nested(
	const char* message)
{
	std::throw_with_nested(MtTaskMgrImplException{message});
}

void MtTaskMgrImpl::initialize_concurrency()
{
	max_threads_ = static_cast<int>(std::thread::hardware_concurrency());

	if (concurrency_reserve_ < 0)
	{
		fail("Concurrency reserve out of range.");
	}

	thread_count_ = max_threads_ - concurrency_reserve_;

	if (thread_count_ <= 0)
	{
		thread_count_ = 1;
	}

	if (thread_count_ > max_threads_)
	{
		thread_count_ = max_threads_;
	}
}

void MtTaskMgrImpl::initialize_threads()
{
	mt_threads_.resize(thread_count_);

	for (auto& mt_thread : mt_threads_)
	{
		mt_thread.is_failed_ = false;
		mt_thread.exception_ = nullptr;

		mt_thread.thread_ = std::thread{&MtTaskMgrImpl::mt_thread_func, this, &mt_thread};
	}
}

void MtTaskMgrImpl::initialize()
{
	initialize_concurrency();
	initialize_threads();
}

void MtTaskMgrImpl::uninitialize()
{
	mt_is_quit_.store(true, std::memory_order_release);

	for (auto& mt_thread : mt_threads_)
	{
		if (mt_thread.thread_.joinable())
		{
			mt_thread.thread_.join();
		}
	}
}

bool MtTaskMgrImpl::mt_is_quit()
{
	return mt_is_quit_.load(std::memory_order_acquire);
}

bool MtTaskMgrImpl::try_pick_and_execute()
{
	auto mt_task = MtTaskPtr{};

	if (!mt_task_queue_.pop(mt_task))
	{
		return false;
	}

	try
	{
		mt_task->execute();
		mt_task->set_completed();
	}
	catch (...)
	{
		mt_task->set_failed(std::current_exception());
	}

	return true;
}

void MtTaskMgrImpl::mt_thread_func(
	MtThread* mt_thread)
{
	const auto sleep_duration_ms = std::chrono::milliseconds{1};

	try
	{
		while (!mt_is_quit())
		{
			if (!try_pick_and_execute())
			{
				std::this_thread::sleep_for(sleep_duration_ms);
			}
		}
	}
	catch (...)
	{
		mt_thread->is_failed_ = true;
		mt_thread->exception_ = std::current_exception();
	}
}

//
// MtTaskMgrImpl
// ==========================================================================


MtTaskMgrUPtr make_mt_task_manager(
	int thread_reserve_count,
	int max_task_count)
{
	return std::make_unique<MtTaskMgrImpl>(thread_reserve_count, max_task_count);
}


} // bstone
