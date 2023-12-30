//
//  ScriptingManager.cpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 06/12/23.
//

#include "ScriptingManager.hpp"

//TODO: Maybe make a version of this constructor that takes in a simple pointer (for local references)
ScriptingManager::ScriptingManager(std::shared_ptr<DotnetHost> host, std::string_view targetAssembly)
{
	this->targetAssembly = targetAssembly;
	this->host = host;
}

std::string ScriptingManager::BuildFullClassPath(const char* targetAssembly, const char* targetNamespace, const char* targetClass)
{
	//I'll make this more readable in the future, and we'll probably accept some heap allocs, for now I want to test if it works being performant
	//Concatenate them as {targetNamespace.targetClass}, {assemblyName}
	//Allocate memory to concatenate the string
	const int MAX_ASSEMBLY_DECL = 2048; //Dedicate 2MBs to the target
	std::string fullClassPath;
	fullClassPath.reserve(MAX_ASSEMBLY_DECL);
	fullClassPath += targetNamespace;
	fullClassPath += '.';
	fullClassPath += targetClass;
	fullClassPath += ", ";
	fullClassPath += targetAssembly;
	return fullClassPath;
}
