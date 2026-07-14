#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace Hush
{

/// Event from the file watcher.
struct FileWatchEvent
{
	enum class Type
	{
		Added,
		Removed,
		Modified,
	};
	Type type;
	std::filesystem::path path;
};

/// Thin wrapper around thomasmonkman::FileWatch for the editor.
/// Runs a background file watcher thread, debounces events,
/// filters out .hcooked/ and self-writes, and provides a thread-safe
/// event queue to drain on the editor tick.
class FileWatcher
{
public:
	using Callback = std::function<void(const FileWatchEvent &)>;

	/// Start watching a directory.
	/// @param rootDir  The project content root directory to watch.
	explicit FileWatcher(std::filesystem::path rootDir);
	~FileWatcher();

	FileWatcher(const FileWatcher &) = delete;
	FileWatcher &operator=(const FileWatcher &) = delete;

	/// Drain all queued events, calling the callback for each.
	/// Must be called from the main thread (editor tick).
	void DrainEvents(Callback callback);

	/// Get the root directory being watched.
	[[nodiscard]]
	const std::filesystem::path &RootDir() const { return m_rootDir; }

	/// Suppress watcher events for a path for a short window. Used by the cooker to
	/// ignore the events caused by its own writes (self-write suppression).
	void SuppressPath(const std::filesystem::path &path);

private:
	void OnFileChanged(const std::filesystem::path &path, bool isAdded, bool isRemoved, bool isModified);

	std::filesystem::path m_rootDir;
	std::unique_ptr<void, void(*)(void*)> m_watcher; // filewatch::FileWatch opaque

	std::mutex m_mutex;
	std::queue<FileWatchEvent> m_eventQueue;

	// Per-path debounce: last accepted event time keyed by absolute path. A single
	// global timestamp would drop distinct events across different files in a burst.
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lastEventTimes;
	// Self-write suppression: ignore events for these paths until the given time point.
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_suppressedUntil;

	static constexpr auto DEBOUNCE_MS = std::chrono::milliseconds(200);
	static constexpr auto SUPPRESS_MS = std::chrono::milliseconds(1000);
};

} // namespace Hush
