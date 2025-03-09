#pragma once

#include <chrono>

namespace Hush {
	/// @brief Measures REAL TIME differences
	class Timer {
	public:
		void Start() {
			m_startTime = std::chrono::high_resolution_clock::now();
		}

		/// @brief Returns the amount of time passed since `Start()` was called in ms
		float Ellapsed() {
			auto endTime = std::chrono::high_resolution_clock::now();
			return std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
		}

		/// @brief Stops the timer and returns the amount of time passed since `Start()` was called in ms
		/// Then proceeds to start the timer again
		float Reset() {
			auto now = std::chrono::high_resolution_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(now - m_startTime).count();
			m_startTime = now;
			return elapsed;
		}

	private:
		std::chrono::high_resolution_clock::time_point m_startTime = std::chrono::high_resolution_clock::now();
	};
}
