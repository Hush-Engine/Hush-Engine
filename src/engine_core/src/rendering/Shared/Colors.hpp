/*! \file Colors.hpp
    \author Kyn21kx
    \date 2024-07-06
    \brief Manages Color definitions for both internal and external APIs
*/

#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace Hush
{
    ///@brief Color definitions for glm
    class Color
    {
      public:
        static constexpr auto s_white = glm::vec4(1, 1, 1, 1);
        
        static constexpr auto s_grey = glm::vec4(0.66f, 0.66f, 0.66f, 1);
        
        static constexpr auto s_black = glm::vec4(0, 0, 0, 0);
        
        static constexpr auto s_magenta = glm::vec4(1, 0, 1, 1);

        static constexpr uint32_t ColorAsInteger(glm::vec4 const &color);

        private:
        static inline constexpr uint32_t CompileTimePackUnorm4x8(glm::vec4 const &v)
        {
            union {
                unsigned char in[4];
                uint32_t out;
            } u = {};

            glm::vec<4, uint8_t, glm::defaultp> result(round(clamp(v, 0.0f, 1.0f) * 255.0f));

            u.in[0] = result[0];
            u.in[1] = result[1];
            u.in[2] = result[2];
            u.in[3] = result[3];

            return u.out;
        }

    };
}