/*! \file TypeUtils.hpp
    \author Kyn21kx
    \date 2024-04-08
    \brief Utility functions for types
*/

#pragma once

class TypeUtils
{
  public:
    template <class B, class D> static bool IsInstanceOf(D instance)
    {
        return typeid(instance) == typeid(B);
    }
};