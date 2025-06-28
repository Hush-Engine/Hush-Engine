#include "ResourceManager.hpp"

Hush::RefCounted* Hush::ResourceManager::IncreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	count.count++;
	return &count;
}


void Hush::ResourceManager::DecreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	// TODO: Add condition to prevent overflow
	count.count--;
	if (count.count == 0) {
		this->m_deletionQueue.emplace_back(handle);
	}
}


const Hush::RefCounted& Hush::ResourceManager::GetRefCount(const Hush::HandleId& handle) {
	return this->m_references[handle];
}
