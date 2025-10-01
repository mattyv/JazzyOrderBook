#pragma once
// select_nth.hpp
// Single-file implementation of "select": index of the n-th set bit in a 64-bit
// mask.
// - Runtime BMI2 detection on x86-64 (uses PDEP + TZCNT if available).
// - Portable fast fallback using byte lookup tables (works on x86-64 and
// ARM64).
//
// API:
//   int select_nth_set_bit(uint64_t mask, unsigned n);
//
// Behavior:
//   - Returns the bit index (0..63) of the n-th set bit in 'mask' (n is
//   zero-based).
//   - Throws std::out_of_range if n >= popcount(mask).
//
// Build examples (Linux):
//   g++ -O3 -std=gnu++20 select_nth.cpp -o select_nth
//   clang++ -O3 -std=gnu++20 select_nth.cpp -o select_nth
//
// Notes:
// - On x86-64, we detect BMI2 using CPUID leaf 7 EBX bit 8.
// - If BMI2 is present, we use _pdep_u64(1<<n, mask) and countr_zero to find
// the index.
// - If not, we use a 256-entry popcount-by-byte table and a 256x8
// select-in-byte table.

#include <array>
#include <bit>
#include <bitset>
#include <cstdint>
#include <stdexcept>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h> // _pdep_u64, TZCNT intrinsics (when compiled with support)
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h> // __get_cpuid_count
#elif defined(_MSC_VER)
#include <intrin.h> // __cpuidex
#endif
#endif

namespace select64 {

// ---------- Tiny tables for the portable path ----------

// Build a 256-entry table where POP8[v] = number of 1-bits in the 8-bit value
// v. Computed at compile-time with constexpr; no runtime construction cost.
constexpr std::array<uint8_t, 256> make_pop8()
{
    std::array<uint8_t, 256> t{};
    for (unsigned int v = 0; v < 256; ++v)
    {
        t[v] = static_cast<uint8_t>(std::popcount(static_cast<uint8_t>(v)));
    }
    return t;
}
constexpr auto POP8 = make_pop8();

// Build a 256x8 table:
// For each 8-bit value v, SEL8[v][k] gives the bit index (0..7) of the k-th
// 1-bit in v, counting from least significant to most significant. If k is out
// of range for v, the entry is filled with 0xFF (unused).
constexpr std::array<std::array<uint8_t, 8>, 256> make_sel8()
{
    std::array<std::array<uint8_t, 8>, 256> T{};
    for (unsigned int v = 0; v < 256; ++v)
    {
        uint8_t x = static_cast<uint8_t>(v);
        uint8_t k = 0;
        for (uint8_t b = 0; b < 8; ++b)
        {
            if (x & (1u << b))
            {
                T[v][k++] = b;
            }
        }
        for (; k < 8; ++k)
            T[v][k] = 0xFF;
    }
    return T;
}
constexpr auto SEL8 = make_sel8();

// Portable select using the byte tables above.
// - Examines at most 8 bytes (fixed small work).
// - Works on any platform.
// Precondition: 0 <= n < popcount(mask), otherwise throws.
inline int select_nth_set_bit_portable(uint64_t mask, unsigned n)
{
    const unsigned total = static_cast<unsigned>(std::popcount(mask));
    if (n >= total)
        throw std::out_of_range("n out of range");

    // Scan bytes from least-significant to most-significant.
    for (int byte = 0; byte < 8; ++byte)
    {
        uint8_t m = static_cast<uint8_t>(mask); // current low byte
        uint8_t pc = POP8[m]; // number of 1-bits in this byte

        if (n < pc)
        {
            // The target is inside this byte: use SEL8 to find which bit.
            uint8_t bit_in_byte = SEL8[m][n];
            return byte * 8 + bit_in_byte;
        }

        // Skip all 1-bits in this byte and move on.
        n -= pc;
        mask >>= 8;
    }

    // With valid inputs, we should never reach here.
    throw std::logic_error("Internal error: exhausted all bytes without finding target bit");
}

// ---------- x86-64: BMI2 detection and fast path ----------

#if defined(__x86_64__) || defined(_M_X64)

// Query CPU features via CPUID to see if BMI2 is available.
// Returns true if BMI2 is supported by the running CPU.
inline bool cpu_has_bmi2()
{
// GCC/Clang on Linux: use __get_cpuid_count. If unavailable, conservatively
// return false.
#if defined(__GNUC__) || defined(__clang__)
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    // CPUID leaf 7, subleaf 0: EBX bit 8 indicates BMI2 support.
    if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
        return false;
    return (ebx & (1u << 8)) != 0;
#elif defined(_MSC_VER)
    int info[4] = {0, 0, 0, 0};
    __cpuidex(info, 7, 0);
    // info[1] contains EBX.
    return (info[1] & (1 << 8)) != 0;
#else
    // On other toolchains, skip CPUID; fallback path will be used.
    return false;
#endif
}

// Fast BMI2-based select using PDEP + countr_zero.
// - _pdep_u64 deposits the single 1-bit from (1 << n) into the n-th 1-bit
// position of 'mask'.
// - countr_zero then returns its index.
// Precondition: 0 <= n < popcount(mask), otherwise throws.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("bmi2")))
#endif
inline int
select_nth_set_bit_bmi2(uint64_t mask, unsigned n)
{
    const unsigned total = static_cast<unsigned>(std::popcount(mask));
    if (n >= total)
        throw std::out_of_range("n out of range");

    // Build a word with a single 1 at position n (safe because n < total <= 64).
    uint64_t single = 1ull << n;

    // Route that bit through the 1-bit positions of mask: result has exactly one
    // 1-bit.
    uint64_t routed = _pdep_u64(single, mask);

    // Index of that single 1-bit equals the number of trailing zeros.
    return std::countr_zero(routed);
}

#endif // x86-64

// ---------- std::bitset support with _Find_next optimization ----------

// Build-time message for implementation choice
#if defined(__GLIBCXX__) || defined(_GLIBCXX_BITSET)
#pragma message("Using libstdc++ with _Find_next() optimization for bitset operations")
#define HAS_FIND_NEXT 1
#else
#pragma message("Using portable bitset implementation without _Find_next()")
#define HAS_FIND_NEXT 0
#endif

// Template function for finding nth set bit in std::bitset
template <size_t N>
[[nodiscard]] inline size_t select_nth_set_bit_unchecked(const std::bitset<N>& bitset, size_t n)
{
#if HAS_FIND_NEXT
    // libstdc++ implementation with _Find_next()
    size_t current_pos = bitset._Find_first();
    for (size_t count = 0; count < n; ++count)
    {
        current_pos = bitset._Find_next(current_pos);
    }
    if (current_pos >= N)
    {
        throw std::out_of_range("n out of range");
    }
    return current_pos;
#else
    // Portable fallback implementation
    size_t count = 0;
    for (size_t i = 0; i < N; ++i)
    {
        if (bitset[i])
        {
            if (count == n)
            {
                return i;
            }
            ++count;
        }
    }
#endif
    throw std::logic_error("Internal error: should not reach here");
}

// Template function for finding nth set bit in std::bitset with bounds check.
// This overload retains the original API and uses the unchecked helper after
// validating that 'n' is within the number of set bits.
template <size_t N>
[[nodiscard]] inline size_t select_nth_set_bit(const std::bitset<N>& bitset, size_t n)
{
    const size_t total = bitset.count();
    if (n >= total)
    {
        throw std::out_of_range("n out of range");
    }

    return select_nth_set_bit_unchecked(bitset, n);
}

// ---------- Public API: pick best available path ----------

// Returns index (0..63) of the n-th set bit; throws if n is out of range.
// - On x86-64 with BMI2: uses PDEP fast path.
// - Otherwise: uses portable byte-table method.
[[nodiscard]] inline int select_nth_set_bit(uint64_t mask, unsigned n)
{
#if defined(__x86_64__) || defined(_M_X64)
    // Use BMI2 if available on this CPU at runtime.
    if (cpu_has_bmi2())
    {
        return select_nth_set_bit_bmi2(mask, n);
    }
    // else fall through to portable path
#endif
    return select_nth_set_bit_portable(mask, n);
}

} // namespace select64
