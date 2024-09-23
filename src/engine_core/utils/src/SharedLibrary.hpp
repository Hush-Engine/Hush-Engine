/*! \file SharedLibrary.hpp
\author Alan Ramirez
    \date 2024-09-22
    \brief Shared Library implementation
*/

#pragma once
#include <string_view>

class SharedLibrary
{
    explicit SharedLibrary(void *handle);

  public:
    SharedLibrary(const SharedLibrary &) = delete;
    SharedLibrary &operator=(const SharedLibrary &) = delete;

    SharedLibrary(SharedLibrary &&rhs) noexcept;
    SharedLibrary &operator=(SharedLibrary &&rhs) noexcept;

    ~SharedLibrary();

    [[nodiscard]] void *GetNativeHandle() const
    {
        return m_nativeHandle;
    }

    template <typename T> [[nodiscard]] T *GetSymbol(std::string_view symbolName)
    {
        return reinterpret_cast<T *>(GetRawSymbol(symbolName));
    }

    /// Opens a shared library and returns a handle to it.
    /// @param libraryName Shared Library name.
    /// @return A handle to the shared library
    static SharedLibrary OpenSharedLibrary(std::string_view libraryName);

  private:
    void *GetRawSymbol(std::string_view symbolName);

    void *m_nativeHandle;
};