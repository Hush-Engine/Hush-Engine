/*! \file SharedLibrary.hpp
\author Alan Ramirez
    \date 2024-09-22
    \brief Shared Library implementation
*/

#pragma once
#include <Result.hpp>
#include <string_view>

namespace Hush
{

    class SharedLibrary
    {
        explicit SharedLibrary(void *handle);

      public:
        /// SharedLibrary error
        enum class EError
        {
            EmptyName,
            NotFound,
            InternalError,
        };

        SharedLibrary(const SharedLibrary &) = delete;
        SharedLibrary &operator=(const SharedLibrary &) = delete;

        SharedLibrary(SharedLibrary &&rhs) noexcept;
        SharedLibrary &operator=(SharedLibrary &&rhs) noexcept;

        ~SharedLibrary();

        [[nodiscard]] void *GetNativeHandle() const
        {
            return m_nativeHandle;
        }

        /// Gets a pointer to a symbol. This call does not check if the pointer is null.
        /// @tparam T Type of the symbol
        /// @param symbolName Name of the symbol
        /// @return A pointer to the symbol
        template <typename T> [[nodiscard]] T *GetSymbolUnsafe(std::string_view symbolName)
        {
            return reinterpret_cast<T *>(GetRawSymbol(symbolName));
        }

        /// Gets a pointer to a symbol.
        /// @tparam T Type of the symbol
        /// @param symbolName Name of the symbol
        /// @return A result with the symbol, or an error if it can't be found
        template <typename T> [[nodiscard]] Result<T, EError> GetSymbol(std::string_view symbolName) noexcept
        {
            auto symbol = GetSymbolUnsafe<T>(symbolName);
            if (symbol == nullptr)
            {
                return EError::NotFound;
            }
            return symbol;
        }

        /// Opens a shared library and returns a handle to it.
        /// @param libraryName Shared Library name.
        /// @return A handle to the shared library
        static Result<SharedLibrary, EError> OpenSharedLibrary(std::string_view libraryName) noexcept;

      private:
        void *GetRawSymbol(std::string_view symbolName);

        void *m_nativeHandle;
    };

}; // namespace Hush