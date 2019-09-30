#ifndef WNAF
#define WNAF

#include "stdint.h"
#include "stddef.h"

namespace barretenberg
{
namespace wnaf
{
constexpr size_t SCALAR_BITS = 127;

#define WNAF_SIZE(x) ((wnaf::SCALAR_BITS + x - 1) / (x))

inline uint32_t get_wnaf_bits(uint64_t *scalar, size_t bits, size_t bit_position)
{
    /**
     *  we want to take a 128 bit scalar and shift it down by (bit_position).
     * We then wish to mask out `bits` number of bits.
     * Low limb contains first 64 bits, so we wish to shift this limb by (bit_position mod 64), which is also (bit_position & 63)
     * If we require bits from the high limb, these need to be shifted left, not right.
     * Actual bit position of bit in high limb = `b`. Desired position = 64 - (amount we shifted low limb by) = 64 - (bit_position & 63)
     * 
     * So, step 1:
     * get low limb and shift right by (bit_position & 63)
     * get high limb and shift left by (64 - (bit_position & 63))
     * 
     * If low limb == high limb, we know that the high limb will be shifted left by a bit count that moves it out of the result mask
     */
    size_t lo_idx = bit_position >> 6;
    size_t hi_idx = (bit_position + bits - 1) >> 6;
    uint32_t lo = (uint32_t)(scalar[lo_idx] >> (bit_position & 63UL));
    size_t hi_mask = 0UL -(~(hi_idx & lo_idx) & 1UL);
    uint32_t hi = (uint32_t)((scalar[hi_idx] << (64UL - (bit_position & 63UL))) & hi_mask);
    return (lo | hi) & ((1U << (uint32_t)bits) - 1U);
}

inline void fixed_wnaf(uint64_t *scalar, uint32_t *wnaf, bool &skew_map, size_t num_points, size_t wnaf_bits)
{
    size_t wnaf_entries = (SCALAR_BITS + wnaf_bits - 1) / wnaf_bits;
    skew_map = ((scalar[0] & 1) == 0);
    uint32_t previous = get_wnaf_bits(scalar, wnaf_bits, 0) + (uint32_t)skew_map;
    for (size_t i = 1; i < wnaf_entries - 1; ++i)
    {
        uint32_t slice = get_wnaf_bits(scalar, wnaf_bits, i * wnaf_bits);
        uint32_t predicate = ((slice & 1U) == 0U);
        wnaf[(wnaf_entries - i) * num_points] = (((previous - (predicate << ((uint32_t)wnaf_bits /*+ 1*/))) ^ (0U - predicate)) >> 1U) | (predicate << 31U);
        previous = slice + predicate;
    }
    size_t final_bits = SCALAR_BITS - (SCALAR_BITS / wnaf_bits) * wnaf_bits;
    uint32_t slice = get_wnaf_bits(scalar, final_bits, (wnaf_entries - 1) * wnaf_bits);
    uint32_t predicate = ((slice & 1U) == 0U);
    wnaf[num_points] = (((previous - (predicate << ((uint32_t)wnaf_bits /*+ 1*/))) ^ (0U - predicate)) >> 1U) | (predicate << 31);
    wnaf[0] = ((slice + predicate) >> 1U);
}

} // namespace wnaf
} // namespace barretenberg

#endif