#pragma once

#include "ICooker.hpp"
#include <array>
#include <memory>
#include <mutex>

namespace Hush::Graphics
{
	class ShaderCompiler;
}

namespace Hush
{

	/// Cooks .slang shader source files into multi-backend .hshader blobs.
	/// Thread-safe: serializes all compiles behind a mutex (Slang global session is not reentrant).
	class ShaderCooker final : public ICooker
	{
	public:
		static constexpr std::array<EFileExtension, 1> EXTENSIONS = {EFileExtension::SLANG};

		explicit ShaderCooker(Graphics::ShaderCompiler *compiler = nullptr);
		~ShaderCooker(); // out-of-line so unique_ptr<ShaderCompiler> sees the complete type

		std::span<const EFileExtension> SupportedExtensions() const override;
		bool CanCook(const FileInfo &info) const override;
		HMeta DefaultMeta(EFileExtension ext, std::string_view srcVPath) const override;
		Result<CookResult, ECookError> Cook(std::span<const std::byte> input, const HMeta &meta,
											const CookContext &ctx) override;

	private:
		/// Borrowed compiler (may be null). When null, m_ownedCompiler is used instead.
		Graphics::ShaderCompiler *m_compiler = nullptr;
		/// Persistent fallback compiler, created on first use and reused across cooks so
		/// Slang is initialized once and its compile cache is shared (instead of being
		/// recreated per cook). Not a borrowed ENGINE_MANAGER component pointer, which flecs
		/// may relocate when the entity's archetype changes.
		std::unique_ptr<Graphics::ShaderCompiler> m_ownedCompiler;
		std::mutex m_mutex;
	};

} // namespace Hush
