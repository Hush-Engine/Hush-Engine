#include "ResourceManager.hpp"

void Hush::ResourceManager::IncreaseRefCount(const HandleId& handle) {
	this->m_references[handle].count++;
}


