//
//  ScriptingManager.cpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//

#include "ScriptingManager.hpp"

#include "log/Logger.hpp"

#include <utility>

// TODO: Maybe make a version of this constructor that takes in a simple pointer (for local references)
ScriptingManager::ScriptingManager(std::shared_ptr<DotnetHost> host, std::string_view targetAssembly)
    : m_host(std::move(host)), m_targetAssembly(targetAssembly)
{
}

std::string ScriptingManager::BuildFullClassPath(const char *targetAssembly, const char *targetNamespace,
                                                 const char *targetClass) const
{
    // Allocate memory to concatenate the string
    const int maxAssemblyDecl = 2048; // Dedicate 2MBs to the target
    std::string fullClassPath;
    fullClassPath.reserve(maxAssemblyDecl);
    fullClassPath += targetNamespace;
    fullClassPath += '.';
    fullClassPath += targetClass;
    fullClassPath += ", ";
    fullClassPath += targetAssembly;
    return fullClassPath;
}
