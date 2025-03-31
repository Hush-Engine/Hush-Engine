#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include <zadeh/StringArrayFilterer.h>
#include <zadeh/filter.h>
#include <zadeh/zadeh.h>
/// @brief Provides utility functions for handling strings (C and std strings)
namespace Hush::ArrayUtils
{

	template <class ArrayType, class ElementType>
	inline std::vector<ElementType> FuzzyFind(const ArrayType& candidates, const std::string& query) {
		zadeh::StringArrayFilterer<ArrayType, ArrayType, ElementType> filterer{};
		filterer.set_candidates(candidates);
		std::vector<size_t> filteredIdx = filterer.filter_indices(query);
		std::vector<ElementType> result{};
		result.reserve(filteredIdx.size());
		for (const size_t& idx : filteredIdx) {
			result.emplace_back(candidates[idx]);			
		}
		return result;
	}

	
	template <class ArrayType, class ElementType>
	inline std::vector<size_t> FuzzyFindIndices(const ArrayType& candidates, const std::string& query) {
		zadeh::StringArrayFilterer<ArrayType, ArrayType, ElementType> filterer{};
		filterer.set_candidates(candidates);
		return filterer.filter_indices(query);
	}
	
}; // namespace Hush::StringUtils
