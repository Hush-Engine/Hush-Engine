#include "FileWatcher.hpp"
#include "Logger.hpp"
#include <thomasmonkman-filewatch/FileWatch.hpp>

#include <exception>
#include <system_error>

namespace Hush
{

FileWatcher::FileWatcher(std::filesystem::path rootDir)
	: m_rootDir(std::move(rootDir)),
	  m_watcher(nullptr, [](void*) {})
{
	// filewatch throws if the watched directory does not exist. Ensure it exists, and
	// degrade gracefully (no live re-cook) instead of aborting the editor if the watch
	// still cannot be established.
	std::error_code ec;
	std::filesystem::create_directories(m_rootDir, ec);

	using FW = filewatch::FileWatch<std::string>;
	try
	{
	auto *fw = new FW(
		m_rootDir.string(),
		[this](const std::string &pathStr, const filewatch::Event eventType) {
			std::filesystem::path path(pathStr);
			// filewatch reports paths relative to the watched root; normalize to absolute
			// so downstream existence checks and .hcooked filtering are correct.
			if (path.is_relative())
			{
				path = m_rootDir / path;
			}

			// Ignore events under .hcooked/
			std::error_code ec;
			auto relPath = std::filesystem::relative(path, m_rootDir, ec);
			if (!ec)
			{
				auto firstComponent = relPath.begin();
				if (firstComponent != relPath.end() && *firstComponent == ".hcooked")
				{
					return;
				}
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				const std::string key = path.generic_string();
				const auto now = std::chrono::steady_clock::now();

				// Self-write suppression (e.g. the cooker's own .hmeta writes).
				auto supIt = m_suppressedUntil.find(key);
				if (supIt != m_suppressedUntil.end())
				{
					if (now < supIt->second)
					{
						return;
					}
					m_suppressedUntil.erase(supIt);
				}

				// Per-path debounce: collapse a save-storm for the same file, but never
				// drop an event for a *different* file.
				auto it = m_lastEventTimes.find(key);
				if (it != m_lastEventTimes.end() && (now - it->second) < DEBOUNCE_MS)
				{
					it->second = now;
					return;
				}
				m_lastEventTimes[key] = now;
			}

			bool isAdded = (eventType == filewatch::Event::added);
			bool isRemoved = (eventType == filewatch::Event::removed);
			bool isModified = (eventType == filewatch::Event::modified);

			OnFileChanged(path, isAdded, isRemoved, isModified);
		});

	m_watcher = std::unique_ptr<void, void(*)(void*)>(
		fw,
		[](void *p) { delete static_cast<filewatch::FileWatch<std::string>*>(p); });

	LogFormat(ELogLevel::Info, "FileWatcher: watching {}", m_rootDir.string());
	}
	catch (const std::exception &e)
	{
		LogFormat(ELogLevel::Error, "FileWatcher: failed to watch {}: {}", m_rootDir.string(), e.what());
	}
}

FileWatcher::~FileWatcher()
{
	// unique_ptr deleter handles destruction
}

void FileWatcher::OnFileChanged(const std::filesystem::path &path, bool isAdded, bool isRemoved, bool /*isModified*/)
{
	FileWatchEvent::Type type;
	if (isAdded) type = FileWatchEvent::Type::Added;
	else if (isRemoved) type = FileWatchEvent::Type::Removed;
	else type = FileWatchEvent::Type::Modified;

	FileWatchEvent ev{type, path};

	std::lock_guard<std::mutex> lock(m_mutex);
	m_eventQueue.push(std::move(ev));
}

void FileWatcher::SuppressPath(const std::filesystem::path &path)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_suppressedUntil[path.generic_string()] = std::chrono::steady_clock::now() + SUPPRESS_MS;
}

void FileWatcher::DrainEvents(Callback callback)
{
	std::queue<FileWatchEvent> snapshot;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		snapshot.swap(m_eventQueue);
	}

	while (!snapshot.empty())
	{
		callback(snapshot.front());
		snapshot.pop();
	}
}

} // namespace Hush
