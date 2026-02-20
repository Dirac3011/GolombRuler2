#pragma GCC optimize("O3,unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")

#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <omp.h>

using namespace std;

typedef __uint128_t bitmask;

// Pruning Table
const int g_min_span[] = {
    0, 0, 1, 2, 4, 6, 9, 13, 18, 23, 29, 36, 44, 53, 63, 73, 84, 94
};

atomic<bool> g_found_at_alpha(false);

bool backtrack(int* current_set, int size, bitmask seen_once, bitmask seen_twice, int n_target, int alpha) {
    if (g_found_at_alpha.load(std::memory_order_relaxed)) return false;

    int last_val = current_set[size - 1];
    int remaining = n_target - size;

    if (last_val + g_min_span[remaining + 1] > alpha) return false;

    // Fast-path for the final element
    if (size == n_target - 1) {
        bitmask new_diffs_bits = 0;
        for (int i = 0; i < size; ++i) {
            new_diffs_bits |= ((bitmask)1 << (alpha - current_set[i]));
        }
        if (seen_twice & new_diffs_bits) return false;
        
        current_set[size] = alpha; 
        g_found_at_alpha.store(true, std::memory_order_relaxed);
        return true; 
    }

    int max_cand = alpha - g_min_span[remaining];

    if (size == n_target - 2) {
        int sym_max = alpha - (current_set[1] - current_set[0]);
        if (sym_max < max_cand) {
            max_cand = sym_max;
        }
    }

    bitmask invalid_cands = 0;
    for (int i = 0; i < size; ++i) {
        invalid_cands |= (seen_twice << current_set[i]);
    }

    bitmask valid_cands = ~invalid_cands;
    
    valid_cands = (valid_cands >> (last_val + 1)) << (last_val + 1);

    while (valid_cands) {
        uint64_t low = (uint64_t)valid_cands;
        int cand;
        
        if (low) {
            cand = __builtin_ctzll(low);
        } else {
            uint64_t high = (uint64_t)(valid_cands >> 64);
            cand = 64 + __builtin_ctzll(high);
        }

        if (cand > max_cand) break;

        valid_cands &= (valid_cands - 1);

        bitmask new_diffs_bits = 0;
        for (int i = 0; i < size; ++i) {
            new_diffs_bits |= ((bitmask)1 << (cand - current_set[i]));
        }

        bitmask next_twice = seen_twice | (seen_once & new_diffs_bits);
        bitmask next_once = seen_once | new_diffs_bits;
        current_set[size] = cand;
        
        if (backtrack(current_set, size + 1, next_once, next_twice, n_target, alpha)) return true;
    }
    
    return false;
}

int main() {
    int n = 17; 
    int alpha = 94; 
    
    cout << "Search | N=" << n << " | Threads=" << omp_get_max_threads() << endl;

    while (true) {
        auto start_time = chrono::high_resolution_clock::now();
        g_found_at_alpha.store(false, std::memory_order_relaxed);
        
        int final_set[25];
        bool success = false;

        vector<int> second_elements;
        int max_s = alpha - g_min_span[n - 1];
        for (int s = 2; s <= max_s; ++s) second_elements.push_back(s);

        #pragma omp parallel
        {
            int thread_set[25];
            thread_set[0] = 1;

            #pragma omp for schedule(dynamic, 1)
            for (int i = 0; i < second_elements.size(); ++i) {
                if (g_found_at_alpha.load(std::memory_order_relaxed)) continue;

                int cand2 = second_elements[i];
                thread_set[1] = cand2;
                bitmask s1_once = ((bitmask)1 << (cand2 - 1));

                int max_c3 = alpha - g_min_span[n - 2];
                for (int cand3 = cand2 + 1; cand3 <= max_c3; ++cand3) {
                    if (g_found_at_alpha.load(std::memory_order_relaxed)) break;

                    int d1 = cand3 - 1;
                    int d2 = cand3 - cand2;
                    
                    bitmask new_diffs = ((bitmask)1 << d1) | ((bitmask)1 << d2);
                    
                    bitmask s2_twice = (s1_once & new_diffs);
                    bitmask s2_once = s1_once | new_diffs;
                    
                    thread_set[2] = cand3;
                    if (backtrack(thread_set, 3, s2_once, s2_twice, n, alpha)) {
                        #pragma omp critical
                        {
                            success = true;
                            for(int k=0; k<n; ++k) final_set[k] = thread_set[k];
                        }
                    }
                }
            }
        }

        if (success) {
            cout << "\nFOUND! alpha=" << alpha << " | [";
            for (int i = 0; i < n; ++i) cout << final_set[i] << (i == n - 1 ? "" : ", ");
            cout << "]" << endl;
            break;
        } else {
            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = end_time - start_time;
            cout << "alpha=" << alpha << " failed (" << elapsed.count() << "s)" << endl;
            alpha++;
        }
    }
    return 0;
}