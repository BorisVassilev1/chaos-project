#pragma once
#include <sys/types.h>
#include <any>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <thread>
#include <vector>

/**
 * Thread pool made for maximum performance and minimum flexibility.
 * Jobs will be added once, then all threads will wait to finish and then the job queue will be cleared.
 *
 */
#include <vector>
class OneShotThreadPool {
	struct Job {
		std::any						data;
		std::function<void(std::any &)> func;
	};

	std::vector<Job>		  job_queue;
	std::atomic_uint_fast32_t jobs_taken = 0;
	std::atomic_uint_fast32_t jobs_done	 = 0;

	std::vector<std::thread> threads;
	uint					 num_threads;
	std::condition_variable	 start_cv;
	std::mutex				 start_mtx;
	volatile bool			 running  = true;
	volatile bool			 has_work = false;

	std::condition_variable done_cv;
	std::mutex				done_mtx;

   public:
	OneShotThreadPool(uint num_threads = std::thread::hardware_concurrency()) : num_threads(num_threads) {
		threads.reserve(num_threads);

		auto worker = [this]() {
			while (running) {
				if (!has_work) {
					std::unique_lock lock(start_mtx);
					start_cv.wait_for(lock, std::chrono::milliseconds(100), [this]() { return !running || has_work; });
				}
				if (!running) return;
				uint_fast32_t job_index = jobs_taken.fetch_add(1, std::memory_order_seq_cst);
				if (job_index >= job_queue.size()) {
					has_work = false;
					continue;
				}

				auto &[j, f] = job_queue[job_index];
				f(j);
				auto done_counter = jobs_done.fetch_add(1, std::memory_order_seq_cst);
				if (done_counter + 1 == job_queue.size()) { done_cv.notify_all(); }
			}
		};

		for (uint i = 0; i < num_threads; ++i) {
			threads.emplace_back(worker);
		}
	}

	~OneShotThreadPool() { stop(); }

	void reset() {
		running = true;
		has_work = false;
		jobs_taken = 0;
		jobs_done = 0;
		job_queue.clear();
	}

	void addJob(std::any &&job, std::function<void(const std::any &)> &&func) {
		job_queue.emplace_back(Job{std::move(job), std::move(func)});
	}

	void start() {
		if (job_queue.empty()) { return; }
		has_work = true;
		start_cv.notify_all();
	}

	void wait() {
		if (job_queue.empty()) return;
		{
			std::unique_lock lock(done_mtx);
			done_cv.wait(lock, [this]() { return jobs_done.load(std::memory_order_acquire) >= job_queue.size(); });
		}
		assert(jobs_done.load(std::memory_order_acquire) == job_queue.size());
		job_queue.clear();
		jobs_taken.store(0, std::memory_order_release);
		jobs_done.store(0, std::memory_order_release);
	}

	void stop() {
		running = false;
		start_cv.notify_all();
		for (auto &thread : threads) {
			if (thread.joinable()) { thread.join(); }
		}
		job_queue.clear();
		jobs_taken.store(0, std::memory_order_release);
		jobs_done.store(0, std::memory_order_release);
		has_work = false;
	}

	inline constexpr uint getNumThreads() const { return num_threads; }
};
