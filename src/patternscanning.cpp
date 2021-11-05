#include "patternscanning.h"
#include "patternbyte.h"
#include <cstring>

auto XKLib::PatternScanning::searchV1(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_fvalues     = pattern.fvalues();
    auto&& matches             = pattern.matches();
    auto old_matches_size      = matches.size();
    auto&& pattern_values      = pattern.values();
    auto pattern_values_size   = pattern_values.size();
    auto pattern_fvalues_size  = pattern_fvalues.size();
    size_t index               = 0;
    size_t start_index         = 0;
    size_t pattern_value_index = 0;
    auto&& pattern_value       = pattern_fvalues[0];
#if (defined(__AVX512F__) || defined(__AVX2__))
    bool is_aligned;
#endif

    do
    {
        do
        {
#if (defined(__AVX512F__) || defined(__AVX2__))

            is_aligned = view_as<size_t>(&data[start_index])
                             % sizeof(PatternByte::simd_value_t) ?
                           false :
                           true;
#endif

#if defined(__AVX512F__)
            if (!pattern_value.unknown
                && !_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(
                    is_aligned ? _mm512_load_si512(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm512_loadu_si512(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    pattern_value.mask),
                  pattern_value.value))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!pattern_value.unknown
                && !_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(
                    is_aligned ? _mm256_load_si256(
                      view_as<PatternByte::simd_value_t*>(
                        &data[start_index])) :
                                 _mm256_loadu_si256(
                                   view_as<PatternByte::simd_value_t*>(
                                     &data[start_index])),
                    pattern_value.mask),
                  pattern_value.value)))
            {
                goto skip;
            }
#else
            if (!pattern_value.unknown
                && pattern_value.value
                     != (*view_as<PatternByte::simd_value_t*>(
                           &data[start_index])
                         & pattern_value.mask))
            {
                goto skip;
            }
#endif
            start_index += pattern_value.var_size;

            pattern_value_index++;

            if (pattern_value_index >= pattern_fvalues_size)
            {
                break;
            }

            pattern_value = pattern_fvalues[pattern_value_index];
        }
        while (true);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index++;
        start_index = index;
    }
    while (index + pattern_values_size <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchV2(XKLib::PatternByte& pattern,
                                      data_t data,
                                      size_t size,
                                      ptr_t baseAddress) -> bool
{
    auto&& pattern_values        = pattern.values();
    auto&& matches               = pattern.matches();
    auto old_matches_size        = matches.size();
    auto pattern_size            = pattern_values.size();
    auto&& vec_known_values      = pattern.vec_known_values();
    auto&& vec_skipper_uk_values = pattern.vec_skipper_uk_values();
    auto vec_known_values_size   = vec_known_values.size();
    size_t index_known_value     = 0;
    size_t start_index           = 0;
    size_t skipper_index         = 0;
    size_t index                 = 0;
    auto&& known_values          = vec_known_values[0];

    do
    {
        do
        {
            do
            {
                if (known_values[index_known_value] != data[start_index])
                {
                    goto skip;
                }

                index_known_value++;
                start_index++;
            }
            while (index_known_value < known_values.size());

            start_index += vec_skipper_uk_values[skipper_index];
            skipper_index++;
            index_known_value = 0;

            if (skipper_index >= vec_known_values_size)
            {
                break;
            }

            known_values = vec_known_values[skipper_index];
        }
        while (true);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index++;
        start_index = index;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

// auto XKLib::PatternScanning::searchAlignedV1(XKLib::PatternByte&
// pattern,
//                                             data_t aligned_data,
//                                             size_t size,
//                                             ptr_t baseAddress) -> bool
//{
//    auto&& pattern_values         = pattern.values();
//    auto&& matches                = pattern.matches();
//    auto old_matches_size         = matches.size();
//    auto&& simd_unknown_values    = pattern.simd_unknown_values();
//    auto&& unknown_value          = simd_unknown_values[0];
//    auto unknown_values_size      = simd_unknown_values.size();
//    auto pattern_size             = pattern_values.size();
//    auto&& fast_aligned_values    = pattern.fast_aligned_values();
//    auto fast_aligned_values_size = fast_aligned_values.size();

//    size_t index               = 0;
//    size_t simd_value_index    = 0;
//    size_t start_index         = 0;
//    size_t unknown_value_index = 0;

//    if ((view_as<uintptr_t>(aligned_data)
//         % sizeof(PatternByte::simd_value_t))
//        != 0)
//    {
//        XKLIB_EXCEPTION("Buffer is not aligned");
//    }

//    /**
//     * Here we are searching for a pattern that was aligned memory
//     * on smid_value_t bits. This is really faster than the previous
//     * methods.
//     */
//    do
//    {
//        /**
//         * Fill unknown bytes by the data we want to compare,
//         * this permits to gain few instructions compared to a mask
//         * every iterations our simd value loaded.
//         */
//        do
//        {
//            std::memcpy(
//              &view_as<data_t>(
//                &fast_aligned_values[unknown_value.simd_index]
//                   .value)[unknown_value.simd_byte_index],
//              &aligned_data[index + unknown_value.data_byte_index],
//              unknown_value.size_to_copy);
//            unknown_value_index++;
//            unknown_value = simd_unknown_values[unknown_value_index];
//        }
//        while (unknown_value_index < unknown_values_size);

//        do
//        {
//#if defined(__AVX512F__)
//            if (!_mm512_cmpeq_epi64_mask(
//                  _mm512_load_si512(view_as<PatternByte::simd_value_t*>(
//                    &aligned_data[start_index])),
//                  fast_aligned_values[simd_value_index].value))
//            {
//                goto skip;
//            }
//#elif defined(__AVX2__)
//            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
//                  _mm256_load_si256(view_as<PatternByte::simd_value_t*>(
//                    &aligned_data[start_index])),
//                  fast_aligned_values[simd_value_index].value)))
//            {
//                goto skip;
//            }
//#else
//            if (*view_as<PatternByte::simd_value_t*>(
//                  &aligned_data[start_index])
//                != fast_aligned_values[simd_value_index].value)
//            {
//                goto skip;
//            }
//#endif
//            start_index += sizeof(PatternByte::simd_value_t);
//            simd_value_index++;
//        }
//        while (simd_value_index < fast_aligned_values_size);

//        matches.push_back(
//          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

//    skip:
//        index += sizeof(PatternByte::simd_value_t);
//        start_index      = index;
//        simd_value_index = 0;
//    }
//    while ((index + pattern_size) <= size);

//    return matches.size() != old_matches_size;
//}

auto XKLib::PatternScanning::searchAlignedV2(XKLib::PatternByte& pattern,
                                             data_t aligned_data,
                                             size_t size,
                                             ptr_t baseAddress) -> bool
{
    auto&& pattern_values          = pattern.values();
    auto&& matches                 = pattern.matches();
    auto old_matches_size          = matches.size();
    auto pattern_size              = pattern_values.size();
    auto fast_aligned_values       = pattern.fast_aligned_values();
    auto fast_aligned_masks        = pattern.fast_aligned_masks();
    auto fast_aligned_values_count = pattern.fast_aligned_values_count();

    size_t index            = 0;
    size_t simd_value_index = 0;
    size_t start_index      = 0;

    if ((view_as<uintptr_t>(aligned_data)
         % sizeof(PatternByte::simd_value_t))
        != 0)
    {
        XKLIB_EXCEPTION("Buffer is not aligned");
    }

    /**
     * Here we are searching for a pattern that was aligned memory
     * on smid_value_t bits. This is really faster than the previous
     * methods.
     */
    do
    {
        do
        {
#if defined(__AVX512F__)
            if (!_mm512_cmpeq_epi64_mask(
                  _mm512_and_si512(_mm512_load_si512(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index]))
            {
                goto skip;
            }
#elif defined(__AVX2__)
            if (!_mm256_movemask_epi8(_mm256_cmpeq_epi64(
                  _mm256_and_si256(_mm256_load_si256(
                                     view_as<PatternByte::simd_value_t*>(
                                       &aligned_data[start_index])),
                                   fast_aligned_masks[simd_value_index]),
                  fast_aligned_values[simd_value_index])))
            {
                goto skip;
            }
#else
            if ((*view_as<PatternByte::simd_value_t*>(
                   &aligned_data[start_index])
                 & fast_aligned_masks[simd_value_index])
                != fast_aligned_values[simd_value_index])
            {
                goto skip;
            }
#endif
            start_index += sizeof(PatternByte::simd_value_t);
            simd_value_index++;
        }
        while (simd_value_index < fast_aligned_values_count);

        matches.push_back(
          view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));

    skip:
        index += sizeof(PatternByte::simd_value_t);
        start_index      = index;
        simd_value_index = 0;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

/**
 * I don't know, and I don't want to know why, but searchV3 should be
 * faster than V4 because
 */
auto XKLib::PatternScanning::searchTest(XKLib::PatternByte& pattern,
                                        data_t data,
                                        size_t size,
                                        ptr_t baseAddress) -> bool
{
    auto&& pattern_values     = pattern.values();
    auto&& matches            = pattern.matches();
    auto old_matches_size     = matches.size();
    auto pattern_size         = pattern_values.size();
    size_t index_pattern_byte = 1;
    size_t index              = 0;

    do
    {
        /**
         * TODO:
         * This would need to be more generic,
         * but it would need a constexpr pattern to generate the if
         * conditions for each known byte.
         */
        if (pattern_values[0]->value == data[index])
        {
            do
            {
                if (pattern_values[index_pattern_byte]->value
                    == PatternByte::Value::UNKNOWN)
                {
                    index_pattern_byte++;
                }
                else if (pattern_values[index_pattern_byte]->value
                         != data[index + index_pattern_byte])
                {
                    index_pattern_byte = 1;
                    goto skip;
                }
                else
                {
                    index_pattern_byte++;
                }
            }
            while (index_pattern_byte < pattern_size);

            matches.push_back(
              view_as<ptr_t>(view_as<uintptr_t>(baseAddress) + index));
        }
    skip:
        index++;
    }
    while ((index + pattern_size) <= size);

    return matches.size() != old_matches_size;
}

auto XKLib::PatternScanning::searchInProcess(
  XKLib::PatternByte& pattern,
  Process& process,
  const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>&
    searchMethod) -> void
{
    auto mmap     = process.mmap();
    auto areaName = pattern.areaName();

    if (areaName.empty())
    {
        for (auto&& area : mmap.areas())
        {
            if (area->isReadable())
            {
                auto area_read = area->read<PatternByte::simd_value_t>();
                searchMethod(pattern,
                             view_as<data_t>(area_read.data()),
                             area->size(),
                             area->begin<ptr_t>());
            }
        }
    }
    else
    {
        searchInProcessWithAreaName(pattern, process, areaName);
    }
}

auto XKLib::PatternScanning::searchInProcessWithAreaName(
  XKLib::PatternByte& pattern,
  Process& process,
  const std::string& areaName,
  const std::function<auto(PatternByte&, data_t, size_t, ptr_t)->bool>&
    searchMethod) -> void
{
    auto mmap = process.mmap();

    for (auto&& area : mmap.areas())
    {
        if (area->isReadable()
            && (area->name().find(areaName) != std::string::npos))
        {
            auto area_read = area->read<PatternByte::simd_value_t>();
            searchMethod(pattern,
                         view_as<data_t>(area_read.data()),
                         area->size(),
                         area->begin<ptr_t>());
        }
    }
}
