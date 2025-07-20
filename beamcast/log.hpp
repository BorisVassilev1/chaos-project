#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <util/utils.hpp>
#include <chrono>

class PercentLogger {
	std::string			name;
	std::size_t			total;
	std::atomic_int64_t current;
	std::size_t percent = 1;

	std::mutex mutex;

   public:
	PercentLogger(const std::string& name, std::size_t total) : name(name), total(total), current(0) {
		std::lock_guard lock(mutex);
		dbLogR(dbg::LOG_INFO, name, ": 0%");
		percent = std::max(total / 100, 1lu);
	}

	inline constexpr void step() {
		current.fetch_add(1, std::memory_order_relaxed);
		if (current.load(std::memory_order_relaxed) % percent == 0) {
			std::lock_guard lock(mutex);
			dbLogR(dbg::LOG_INFO, name, ": ", (current * 100 / total), "%");
		}
	}

	inline constexpr void finish() {
		std::lock_guard lock(mutex);
		dbLogR(dbg::LOG_INFO, name, " : 100%");
	}
};

class Timer {
	std::chrono::high_resolution_clock::time_point start;

   public:
	Timer() : start(std::chrono::high_resolution_clock::now()) {}

	template<typename T>
	auto elapsed() const {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<T>(end - start);
		return duration.count();
	}
};
