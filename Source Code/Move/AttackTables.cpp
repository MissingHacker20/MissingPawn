#include "Move/AttackTables.h"

#include <array>
#include <cstdint>
#include <bit>
#include <iostream>
#include <vector>

#if defined(_MSC_VER)
#include <immintrin.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#endif

namespace
{

constexpr int bishopDirs[4][2] = {{1,1}, {1,-1}, {-1,-1}, {-1,1}};
constexpr int rookDirs[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};

std::array<Bitboard, 64> bishopMasks{};
std::array<Bitboard, 64> rookMasks{};

// When true the CPU supports BMI2 and we use the PEXT instruction for the
// fastest possible sliding-piece attack indexing. Otherwise we fall back to
// a portable bit-unpacking scheme that is correct on every architecture.
bool usePext = false;
bool initialized = false;

// Attack tables indexed by the (PEXT- or packBits-) compressed occupancy.
// Each square owns a contiguous slice of the buffer sized 1 << popcount(mask).
std::vector<Bitboard> bishopAttackTable;
std::vector<Bitboard> rookAttackTable;
std::array<size_t, 64> bishopOffsets{};
std::array<size_t, 64> rookOffsets{};

void computeMasks()
{
    for (int sq = 0; sq < 64; ++sq)
    {
        int f = sq % 8;
        int r = sq / 8;

        Bitboard bmask = 0;
        for (int d = 0; d < 4; ++d)
        {
            int nf = f + bishopDirs[d][0];
            int nr = r + bishopDirs[d][1];
            while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
            {
                bmask |= (1ULL << (nr * 8 + nf));
                nf += bishopDirs[d][0];
                nr += bishopDirs[d][1];
            }
        }
        bishopMasks[sq] = bmask;

        Bitboard rmask = 0;
        for (int d = 0; d < 4; ++d)
        {
            int nf = f + rookDirs[d][0];
            int nr = r + rookDirs[d][1];
            while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
            {
                rmask |= (1ULL << (nr * 8 + nf));
                nf += rookDirs[d][0];
                nr += rookDirs[d][1];
            }
        }
        rookMasks[sq] = rmask;
    }
}

// Reference (slow, brute force) sliding attack generation used to build and
// verify the tables. These do not depend on any table state.
Bitboard referenceBishopAttacks(int sq, Bitboard occ)
{
    int f = sq % 8;
    int r = sq / 8;
    Bitboard attacks = 0;
    for (int d = 0; d < 4; ++d)
    {
        int nf = f + bishopDirs[d][0];
        int nr = r + bishopDirs[d][1];
        while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
        {
            int target = nr * 8 + nf;
            attacks |= (1ULL << target);
            if (occ & (1ULL << target)) break;
            nf += bishopDirs[d][0];
            nr += bishopDirs[d][1];
        }
    }
    return attacks;
}

Bitboard referenceRookAttacks(int sq, Bitboard occ)
{
    int f = sq % 8;
    int r = sq / 8;
    Bitboard attacks = 0;
    for (int d = 0; d < 4; ++d)
    {
        int nf = f + rookDirs[d][0];
        int nr = r + rookDirs[d][1];
        while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
        {
            int target = nr * 8 + nf;
            attacks |= (1ULL << target);
            if (occ & (1ULL << target)) break;
            nf += rookDirs[d][0];
            nr += rookDirs[d][1];
        }
    }
    return attacks;
}

// Portable index into the sliding-piece occupancy: pack the relevant occupancy
// bits (those inside the mask) into a dense integer. This is the classic
// "no-magic" look-up used when PEXT is unavailable.
inline uint64_t packBits(uint64_t bits, uint64_t mask)
{
    uint64_t index = 0;
    int shift = 0;
    uint64_t m = mask;
    while (m)
    {
        int b = std::countr_zero(m);
        if (bits & (1ULL << b)) index |= (1ULL << shift);
        ++shift;
        m &= m - 1;
    }
    return index;
}

// _pext_u64 is available from <immintrin.h> on MSVC builds (with /arch:AVX2,
// already set in CMake) and on GCC/Clang when targeting BMI2. We only emit the
// PEXT path when the intrinsic can actually be compiled.
#if defined(__BMI2__) || (defined(_MSC_VER) && !defined(__aarch64__) && !defined(__arm__))
#define HAS_PEXT_INTRIN 1
#else
#define HAS_PEXT_INTRIN 0
#endif

bool detectPextSupport()
{
#if defined(__BMI2__)
    return true;
#elif defined(_MSC_VER)
    #if defined(_WIN32)
        // PF_BMI2_INSTRUCTIONS_AVAILABLE == 22
        return IsProcessorFeaturePresent(22) != 0;
    #else
        return false;
    #endif
#else
    #if defined(__x86_64__) || defined(__i386__)
        #if defined(__GNUC__)
            return __builtin_cpu_supports("bmi2") != 0;
        #else
            return false;
        #endif
    #else
        return false;
    #endif
#endif
}

void generatePawnTable(std::array<Bitboard, 64>& whitePawnAttackTable, std::array<Bitboard, 64>& blackPawnAttackTable)
{
    for (int sq = 0; sq < 64; ++sq)
    {
        int f = sq % 8;
        int r = sq / 8;

        // Białe pionki na polu sq atakują:
        // - sq + 7 (lewy skos, jeśli nie w kolumnie A i nie na 8. rzędzie)
        // - sq + 9 (prawy skos, jeśli nie w kolumnie H i nie na 8. rzędzie)
        if (r < 7)
        {
            if (f > 0) setBit(whitePawnAttackTable[sq], static_cast<Square>(sq + 7));
            if (f < 7) setBit(whitePawnAttackTable[sq], static_cast<Square>(sq + 9));
        }

        // Czarne pionki na polu sq atakują:
        // - sq - 9 (lewy skos, jeśli nie w kolumnie A i nie na 1. rzędzie)
        // - sq - 7 (prawy skos, jeśli nie w kolumnie H i nie na 1. rzędzie)
        if (r > 0)
        {
            if (f > 0) setBit(blackPawnAttackTable[sq], static_cast<Square>(sq - 9));
            if (f < 7) setBit(blackPawnAttackTable[sq], static_cast<Square>(sq - 7));
        }
    }
}

constexpr int knightOffsets[8][2] = {
    {1, 2}, {2, 1}, {2, -1}, {1, -2},
    {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
};

void generateKnightTable(std::array<Bitboard, 64>& knightAttackTable)
{
    for (int sq = 0; sq < 64; ++sq)
    {
        int f = sq % 8;
        int r = sq / 8;

        for (const auto& offset : knightOffsets)
        {
            int nf = f + offset[0];
            int nr = r + offset[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
            {
                setBit(knightAttackTable[sq], static_cast<Square>(nr * 8 + nf));
            }
        }
    }
}

void generateKingTable(std::array<Bitboard, 64>& kingAttackTable)
{
    for (int sq = 0; sq < 64; ++sq)
    {
        int f = sq % 8;
        int r = sq / 8;

        for (int df = -1; df <= 1; ++df)
        {
            for (int dr = -1; dr <= 1; ++dr)
            {
                if (df == 0 && dr == 0) continue;

                int nf = f + df;
                int nr = r + dr;
                if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
                {
                    setBit(kingAttackTable[sq], static_cast<Square>(nr * 8 + nf));
                }
            }
        }
    }
}

inline uint64_t slidingIndex(Bitboard occupancy, Bitboard mask)
{
#if HAS_PEXT_INTRIN
    if (usePext)
    {
        return _pext_u64(occupancy, mask);
    }
#endif
    return packBits(occupancy, mask);
}

void fillSlidingTables()
{
    // Precompute every sliding-piece attack set, indexed by the compressed
    // occupancy (PEXT on BMI2, otherwise packBits). This yields an O(1) lookup
    // with no magic-number search and no collisions.
    auto enumerate = [](Bitboard mask) -> std::vector<Bitboard>
    {
        std::vector<Bitboard> occs;
        int bits = std::popcount(mask);
        int combos = 1 << bits;
        occs.reserve(combos);
        for (int i = 0; i < combos; ++i)
        {
            Bitboard occ = 0;
            int bit = 0;
            uint64_t m = mask;
            while (m)
            {
                int b = std::countr_zero(m);
                if (i & (1 << bit)) occ |= (1ULL << b);
                ++bit;
                m &= m - 1;
            }
            occs.push_back(occ);
        }
        return occs;
    };

    for (int sq = 0; sq < 64; ++sq)
    {
        auto bishopOccs = enumerate(bishopMasks[sq]);
        size_t bSize = static_cast<size_t>(1) << std::popcount(bishopMasks[sq]);
        bishopOffsets[sq] = bishopAttackTable.size();
        bishopAttackTable.resize(bishopAttackTable.size() + bSize, 0);
        for (const auto& occ : bishopOccs)
        {
            uint64_t index = slidingIndex(occ, bishopMasks[sq]);
            bishopAttackTable[bishopOffsets[sq] + index] = referenceBishopAttacks(sq, occ);
        }

        auto rookOccs = enumerate(rookMasks[sq]);
        size_t rSize = static_cast<size_t>(1) << std::popcount(rookMasks[sq]);
        rookOffsets[sq] = rookAttackTable.size();
        rookAttackTable.resize(rookAttackTable.size() + rSize, 0);
        for (const auto& occ : rookOccs)
        {
            uint64_t index = slidingIndex(occ, rookMasks[sq]);
            rookAttackTable[rookOffsets[sq] + index] = referenceRookAttacks(sq, occ);
        }
    }
}

}

namespace AttackTables
{

std::array<Bitboard, 64> whitePawnAttackTable{};
std::array<Bitboard, 64> blackPawnAttackTable{};
std::array<Bitboard, 64> knightAttackTable{};
std::array<Bitboard, 64> kingAttackTable{};

void initAttackTables()
{
    if (initialized) return;
    initialized = true;

    usePext = detectPextSupport();

    computeMasks();
    fillSlidingTables();

    generatePawnTable(whitePawnAttackTable, blackPawnAttackTable);
    generateKnightTable(knightAttackTable);
    generateKingTable(kingAttackTable);
}

Bitboard bishopAttacks(Square square, Bitboard occupancy)
{
    int s = static_cast<int>(square);
    Bitboard masked = occupancy & bishopMasks[s];
    uint64_t index = slidingIndex(masked, bishopMasks[s]);
    return bishopAttackTable[bishopOffsets[s] + index];
}

Bitboard rookAttacks(Square square, Bitboard occupancy)
{
    int s = static_cast<int>(square);
    Bitboard masked = occupancy & rookMasks[s];
    uint64_t index = slidingIndex(masked, rookMasks[s]);
    return rookAttackTable[rookOffsets[s] + index];
}

Bitboard queenAttacks(Square square, Bitboard occupancy)
{
    return bishopAttacks(square, occupancy) | rookAttacks(square, occupancy);
}

bool verifyAttackTables()
{
    auto enumerate = [](Bitboard mask) -> std::vector<Bitboard>
    {
        std::vector<Bitboard> occs;
        int bits = std::popcount(mask);
        int combos = 1 << bits;
        occs.reserve(combos);
        for (int i = 0; i < combos; ++i)
        {
            Bitboard occ = 0;
            int bit = 0;
            uint64_t m = mask;
            while (m)
            {
                int b = std::countr_zero(m);
                if (i & (1 << bit)) occ |= (1ULL << b);
                ++bit;
                m &= m - 1;
            }
            occs.push_back(occ);
        }
        return occs;
    };

    for (int sq = 0; sq < 64; ++sq)
    {
        auto occs = enumerate(bishopMasks[sq]);
        for (const auto& occ : occs)
        {
            if (bishopAttacks(static_cast<Square>(sq), occ) != referenceBishopAttacks(sq, occ))
            {
                std::cerr << "Bishop mismatch at square " << sq << "\n";
                return false;
            }
        }
        auto roccs = enumerate(rookMasks[sq]);
        for (const auto& occ : roccs)
        {
            if (rookAttacks(static_cast<Square>(sq), occ) != referenceRookAttacks(sq, occ))
            {
                std::cerr << "Rook mismatch at square " << sq << "\n";
                return false;
            }
        }
    }
    return true;
}

}
