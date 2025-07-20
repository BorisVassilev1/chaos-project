#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <util/utils.hpp>

class PercentLogger {
	std::string			name;
	std::size_t			total;
	std::atomic_int64_t current;

	std::mutex mutex;

   public:
	PercentLogger(const std::string& name, std::size_t total) : name(name), total(total), current(0) {
		std::lock_guard lock(mutex);
		dbLogR(dbg::LOG_INFO, name, ": 0%");
	}

	inline constexpr void step() {
		current.fetch_add(1, std::memory_order_relaxed);
		if (current.load(std::memory_order_relaxed) % (total / 100) == 0) {
			std::lock_guard lock(mutex);
			dbLogR(dbg::LOG_INFO, name, ": ", (current * 100 / total), "%");
		}
	}

	inline constexpr void finish() {
		std::lock_guard lock(mutex);
		dbLogR(dbg::LOG_INFO, name, " : 100%");
	}
};
