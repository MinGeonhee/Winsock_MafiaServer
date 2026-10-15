#pragma once

namespace Net
{
	template<class T>
	class CEventQueue
	{
	public:
		CEventQueue<T>()
			: m_mutex{}, m_queueEvents{}
		{}
		~CEventQueue<T>() {
		}
	public:
		void	Push_Event(T&& tEvent) 
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queueEvents.push(std::move(tEvent));
		}
		void	Push_Event(const T& tEvent)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queueEvents.push(tEvent);
		}
		bool	Pop_Event(T& outEvent)
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_queueEvents.empty()) return false;
			outEvent = std::move(m_queueEvents.front());
			m_queueEvents.pop();

			return true;
		}
		bool				Is_Empty() { return m_queueEvents.empty(); }
	private:
		std::mutex		m_mutex;
		std::queue<T>	m_queueEvents;
	};
}