/*! \file ComponentTraits.hpp
    \author Alan Ramirez
    \date 2025-01-26
    \brief Component traits
*/

#pragma once

#include "EnumFlags.hpp"
#include "Platform.hpp"

#include <Result.hpp>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace Hush
{
    namespace ComponentTraits
    {
        enum class EComponentOpsFlags : std::uint32_t;
    }
    template <>
    struct EnableBitMaskOperators<ComponentTraits::EComponentOpsFlags> : std::true_type
    {
    };
} // namespace Hush

namespace Hush::ComponentTraits
{
    struct ComponentInfo;

    using ComponentCtor = void (*)(void *array, std::int32_t count, const ComponentInfo &info);
    using ComponentDtor = void (*)(void *array, std::int32_t count, const ComponentInfo &info);
    using ComponentCopy = void (*)(void *dst, const void *src, std::int32_t count, const ComponentInfo &info);
    using ComponentMove = void (*)(void *dst, void *src, std::int32_t count, const ComponentInfo &info);
    using ComponentCopyCtor = void (*)(void *dst, const void *src, std::int32_t count, const ComponentInfo &info);
    using ComponentMoveCtor = void (*)(void *dst, void *src, std::int32_t count, const ComponentInfo &info);

    enum class EComponentOpsFlags : std::uint32_t
    {
        None = 0,

        // Flags used for testing if a component has a specific operation. Do not use these to disable operations. It
        // won't work.
        HasCtor = 1 << 0,
        HasDtor = 1 << 1,
        HasCopy = 1 << 2,
        HasMove = 1 << 3,
        HasCopyCtor = 1 << 4,
        HasMoveCtor = 1 << 5,
        HasMoveDtor = 1 << 6,
        HasMoveAssignDtor = 1 << 7,

        // Flags used to disable operations.
        NoCtor = 1 << 8,
        NoDtor = 1 << 9,
        NoCopy = 1 << 10,
        NoMove = 1 << 11,
        NoCopyCtor = 1 << 12,
        NoMoveCtor = 1 << 13,
        NoMoveDtor = 1 << 14,
        NoMoveAssignDtor = 1 << 15,
    };

    struct ComponentOps
    {
        ComponentCtor ctor;
        ComponentDtor dtor;
        ComponentCopy copy;
        ComponentMove move;
        ComponentCopyCtor copyCtor;
        ComponentMoveCtor moveCtor;
        ComponentMoveCtor moveDtor;
        ComponentMoveCtor moveAssignDtor;
    };

    struct ComponentInfo
    {
        std::size_t size;
        std::size_t alignment;
        std::string_view name;
        ComponentOps ops;
        EComponentOpsFlags opsFlags;

        void *userCtx;
        void (*userCtxFree)(void *);
    };

    template <typename T>
    void CtorImpl(void *array, std::int32_t count, const ComponentInfo &)
    {
        T *ptr = static_cast<T *>(array);
        for (std::int32_t i = 0; i < count; ++i)
        {
            new (ptr + i) T{};
        }
    }

    template <typename T>
    void DtorImpl(void *array, std::int32_t count, const ComponentInfo &)
    {
        T *ptr = static_cast<T *>(array);
        for (std::int32_t i = 0; i < count; ++i)
        {
            ptr[i].~T();
        }
    }

    template <typename T>
    void CopyImpl(void *dst, const void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        const T *srcPtr = static_cast<const T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            dstPtr[i] = srcPtr[i];
        }
    }

    template <typename T>
    void MoveImpl(void *dst, void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        T *srcPtr = static_cast<T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            dstPtr[i] = std::move(srcPtr[i]);
        }
    }

    template <typename T>
    void CopyCtorImpl(void *dst, const void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        const T *srcPtr = static_cast<const T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            new (dstPtr + i) T{srcPtr[i]};
        }
    }

    template <typename T>
    void MoveCtorImpl(void *dst, void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        T *srcPtr = static_cast<T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            new (dstPtr + i) T{std::move(srcPtr[i])};
        }
    }

    template <typename T>
    void MoveCtorDtorImpl(void *dst, void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        T *srcPtr = static_cast<T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            new (dstPtr + i) T{std::move(srcPtr[i])};
            srcPtr[i].~T();
        }
    }

    template <typename T>
    void MoveAssignDtorImpl(void *dst, void *src, std::int32_t count, const ComponentInfo &)
    {
        T *dstPtr = static_cast<T *>(dst);
        T *srcPtr = static_cast<T *>(src);
        for (std::int32_t i = 0; i < count; ++i)
        {
            dstPtr[i] = std::move(srcPtr[i]);
            srcPtr[i].~T();
        }
    }

    /// Get the constructor function for a component.
    /// @tparam T Type of the component.
    /// @return Function pointer to the function that will construct the component.
    template <typename T>
        requires std::is_default_constructible_v<T>
    constexpr ComponentCtor GetCtorImpl(EComponentOpsFlags &)
    {
        return &CtorImpl<T>;
    }

    /// Get the constructor function for a component that does not have a trivial constructor.
    /// @tparam T Type of the component.
    /// @return Nullptr
    template <typename T>
        requires std::is_trivially_constructible_v<T> && !std::is_default_constructible_v<T>
                                                     constexpr ComponentCtor GetCtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires !std::is_default_constructible_v<T>
                 constexpr ComponentCtor GetCtorImpl(EComponentOpsFlags & flags)
    {
        flags |= EComponentOpsFlags::NoCtor;
        return nullptr;
    }

    template <typename T>
        requires std::is_destructible_v<T> && !std::is_trivially_destructible_v<T>
                                          constexpr ComponentDtor GetDtorImpl(EComponentOpsFlags &)
    {
        return &DtorImpl<T>;
    }

    template <typename T>
        requires std::is_trivially_destructible_v<T>
    constexpr ComponentDtor GetDtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires !std::is_destructible_v<T>
                 constexpr ComponentDtor GetDtorImpl(EComponentOpsFlags & flags)
    {
        static_assert(std::is_destructible_v<T>, "Component must be destructible");
        flags |= EComponentOpsFlags::NoDtor;
        return nullptr;
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    constexpr ComponentCopy GetCopyImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires !std::is_trivially_copyable_v<T> && std::is_copy_assignable_v<T>
    constexpr ComponentCopy GetCopyImpl(EComponentOpsFlags &)
    {
        return &CopyImpl<T>;
    }

    template <typename T>
        requires !(std::is_trivially_copyable_v<T> || std::is_copy_assignable_v<T>)
                 constexpr ComponentCopy GetCopyImpl(EComponentOpsFlags & flags)
    {
        flags |= EComponentOpsFlags::NoCopy;
        return nullptr;
    }

    template <typename T>
        requires std::is_trivially_move_assignable_v<T>
    constexpr ComponentMove GetMoveImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires(std::is_move_assignable_v<T> && !std::is_trivially_move_assignable_v<T>)
    constexpr ComponentMove GetMoveImpl(EComponentOpsFlags &)
    {
        return &MoveImpl<T>;
    }

    template <typename T>
        requires(!std::is_move_assignable_v<T>)
    constexpr ComponentMove GetMoveImpl(EComponentOpsFlags &flags)
    {
        flags |= EComponentOpsFlags::NoMove;
        return nullptr;
    }

    template <typename T>
        requires std::is_trivially_copy_constructible_v<T>
    constexpr ComponentCopyCtor GetCopyCtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires(std::is_copy_constructible_v<T> && !std::is_trivially_copy_constructible_v<T>)
    constexpr ComponentCopyCtor GetCopyCtorImpl(EComponentOpsFlags &)
    {
        return &CopyCtorImpl<T>;
    }

    template <typename T>
        requires(!std::is_copy_constructible_v<T>)
    constexpr ComponentCopyCtor GetCopyCtorImpl(EComponentOpsFlags &flags)
    {
        flags |= EComponentOpsFlags::NoCopyCtor;
        return nullptr;
    }

    template <typename T>
        requires std::is_trivially_move_constructible_v<T>
    constexpr ComponentMoveCtor GetMoveCtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires(std::is_move_constructible_v<T> && !std::is_trivially_move_constructible_v<T>)
    constexpr ComponentMoveCtor GetMoveCtorImpl(EComponentOpsFlags &)
    {
        return &MoveCtorImpl<T>;
    }

    template <typename T>
        requires(!std::is_move_constructible_v<T>)
    constexpr ComponentMoveCtor GetMoveCtorImpl(EComponentOpsFlags &flags)
    {
        flags |= EComponentOpsFlags::NoMoveCtor;
        return nullptr;
    }

    template <typename T>
        requires !(std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>) &&
                 (std::is_move_constructible_v<T> && std::is_destructible_v<T>)
    constexpr ComponentMoveCtor GetMoveDtorImpl(EComponentOpsFlags &)
    {
        return &MoveCtorDtorImpl<T>;
    }

    template <typename T>
        requires(std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>)
    constexpr ComponentMoveCtor GetMoveDtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires(!(std::is_move_constructible_v<T> && std::is_destructible_v<T>))
    constexpr ComponentMoveCtor GetMoveDtorImpl(EComponentOpsFlags &flags)
    {
        flags |= EComponentOpsFlags::NoMoveDtor;
        return nullptr;
    }

    template <typename T>
        requires(std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>)
    constexpr ComponentMoveCtor GetMoveAssignDtorImpl(EComponentOpsFlags &)
    {
        return nullptr;
    }

    template <typename T>
        requires !(std::is_move_assignable_v<T> && std::is_destructible_v<T>)
                 constexpr ComponentMoveCtor GetMoveAssignDtorImpl(EComponentOpsFlags & flags)
    {
        flags |= EComponentOpsFlags::NoMoveAssignDtor;
        return nullptr;
    }

    template <typename T>
        requires !(std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>) &&
                 (std::is_move_assignable_v<T> && std::is_destructible_v<T>)
    constexpr ComponentMoveCtor GetMoveAssignDtorImpl(EComponentOpsFlags &)
    {
        return &MoveAssignDtorImpl<T>;
    }

    /// Get the operations for a component.
    /// @tparam T Type of the component.
    /// @return Operations for the component.
    template <typename T>
        requires !std::is_reference_v<T>
                 constexpr ComponentOps GetOps(EComponentOpsFlags & flags)
    {
        return ComponentOps{
            .ctor = GetCtorImpl<std::remove_cvref_t<T>>(flags),
            .dtor = GetDtorImpl<std::remove_cvref_t<T>>(flags),
            .copy = GetCopyImpl<std::remove_cvref_t<T>>(flags),
            .move = GetMoveImpl<std::remove_cvref_t<T>>(flags),
            .copyCtor = GetCopyCtorImpl<std::remove_cvref_t<T>>(flags),
            .moveCtor = GetMoveCtorImpl<std::remove_cvref_t<T>>(flags),
            .moveDtor = GetMoveDtorImpl<std::remove_cvref_t<T>>(flags),
        };
    }

    template <typename T>
    ComponentInfo GetComponentInfo()
    {
        auto componentInfo = ComponentInfo{
            .size = sizeof(T),
            .alignment = alignof(T),
            .name = typeid(T).name(),
            .userCtx = nullptr,
            .userCtxFree = nullptr,
        };

        componentInfo.ops = GetOps<T>(componentInfo.opsFlags);

        return componentInfo;
    }

    // Create a C++20 template that returns the name of a type as a string_view. Based on PRETTY_FUNCTION and some
    // string manipulation.

    namespace detail
    {
#if HUSH_COMPILER_CLANG || HUSH_COMPILER_GCC
        consteval std::string_view ExtractTypeFromPrettyFunction(std::string_view function)
        {
            // Example of function: consteval std::string_view GetTypeName() [with T = int; std::string_view =
            // std::basic_string_view<char>]. We must extract T
            std::size_t start = function.find("T = ");
            if (start == std::string_view::npos)
            {
                return "Unknown";
            }
            start += 4;
            std::size_t end = function.find(";", start);
            if (end == std::string_view::npos)
            {
                return "Unknown";
            }
            return function.substr(start, end - start);
        }
#elif HUSH_COMPILER_MSVC
        consteval std::string_view ExtractTypeFromFuncSig(std::string_view function)
        {
            // Example of function: consteval std::string_view GetTypeName<int>(void)
            // We must extract int
            std::size_t start = function.find("GetTypeName<");
            if (start == std::string_view::npos)
            {
                return "Unknown";
            }
            start += 12;
            // Find the last > character of the string
            std::size_t end = function.find_last_of(">");
            if (end == std::string_view::npos)
            {
                return "Unknown";
            }

            // Remove any struct/class/enum/union prefix

            return function.substr(start, end - start);
        }
#endif

        enum class EEntityRegisterStatus
        {
            Registered,
            NotRegistered,
        };

        /// Get the entity id for a given entity type.
        /// @tparam T Type of the entity, without cvref.
        /// @param worldPtr Pointer to the world.
        /// @return Pair with the status of the entity and the entity id.
        template <typename T>
        static std::pair<EEntityRegisterStatus, std::uint64_t *> GetEntityIdImpl(const void *worldPtr)
        {
            thread_local std::uint64_t entityId = 0;
            thread_local const void *world = nullptr;

            // First, check if it's already registered
            if (entityId == 0 || world != worldPtr)
            {
                world = worldPtr;
                return {EEntityRegisterStatus::NotRegistered, &entityId};
            }
            world = worldPtr;

            return {EEntityRegisterStatus::Registered, &entityId};
        }

        /// Get the entity id for a given entity type.
        /// @tparam T Type of the entity.
        /// @param worldPtr Pointer to the world.
        /// @return Pair with the status of the entity and the entity id.
        template <typename T>
        static std::pair<EEntityRegisterStatus, std::uint64_t *> GetEntityId(const void *worldPtr)
        {
            return GetEntityIdImpl<std::remove_cvref_t<T>>(worldPtr);
        }
    } // namespace detail

    template <typename T>
    consteval std::string_view GetTypeName()
    {
        // Get the function name
#if HUSH_COMPILER_CLANG || HUSH_COMPILER_GCC
        std::string_view name = __PRETTY_FUNCTION__;
#elif HUSH_COMPILER_MSVC
        std::string_view name = __FUNCSIG__;
#endif
        return "Unknown";
    }
} // namespace Hush::ComponentTraits
