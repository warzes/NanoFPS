#pragma once

// TODO: удалить chrono

class Timer final
{
public:
	Timer() { Reset(); }
	void Reset() { m_startTime = m_clock.now(); }

	uint64_t Seconds() const { return time<std::chrono::seconds>(); }
	uint64_t Milliseconds() const { return time<std::chrono::milliseconds>(); }
	uint64_t Microseconds() const { return time<std::chrono::microseconds>(); }

private:
	template<typename T>
	uint64_t time() const
	{
		return static_cast<uint64_t>(std::chrono::duration_cast<T>(m_clock.now() - m_startTime).count());
	}

	std::chrono::high_resolution_clock                          m_clock;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};