#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

typedef __uint128_t bitmask;

const int TARGET_N = 15;
int g_best_alpha = 85; 
std::vector<int> g_best_set;

const int g_min_span[] = {0, 0, 1, 2, 4, 6, 9, 13, 18, 23, 29, 36, 44, 53, 63, 73, 83, 93};

void dfs(std::vector<int>& current_set, bitmask seen_once, bitmask seen_twice) {
    int size = current_set.size();
    int last_val = current_set.back();

    if (size == TARGET_N) {
        if (last_val < g_best_alpha) {
            g_best_alpha = last_val;
            g_best_set = current_set;
            std::cout << "\n[!] NEW RECORD FOUND! alpha=" << g_best_alpha << " | [";
            for(int i=0; i < TARGET_N; ++i) 
                std::cout << g_best_set[i] << (i == TARGET_N - 1 ? "" : ", ");
            std::cout << "]" << std::endl;
        }
        return;
    }

    int remaining = TARGET_N - size;
    if (last_val + g_min_span[remaining + 1] - g_min_span[1] >= g_best_alpha) {
        return;
    }

    int max_cand = g_best_alpha - (g_min_span[remaining] - g_min_span[1]);

    for (int cand = last_val + 1; cand < max_cand; ++cand) {
        // Status update for the top-level branch
        if (size == 1) {
            std::cout << "\rChecking branches where second element is: " << cand << " (Current Best Alpha: " << g_best_alpha << ")... " << std::flush;
        }

        bitmask new_diffs = 0;
        bool conflict = false;
        for (int val : current_set) {
            int d = cand - val;
            if ((seen_twice >> d) & 1) { conflict = true; break; }
            new_diffs |= ((bitmask)1 << d);
        }

        if (!conflict) {
            current_set.push_back(cand);
            dfs(current_set, seen_once | new_diffs, seen_twice | (seen_once & new_diffs));
            current_set.pop_back();
        }
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    int initial_target = g_best_alpha;

    std::cout << "Starting search for N=15. Initial upper bound: " << initial_target << "\n";

    std::vector<int> current_set = {1};
    dfs(current_set, 0, 0);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "\n\n" << std::string(50, '=') << "\n";
    std::cout << "SEARCH FINISHED EXHAUSTIVELY.\n";
    
    // This part explains what happened
    if (g_best_alpha < initial_target) {
        std::cout << "The best alpha found was " << g_best_alpha << ".\n";
        std::cout << "Every possible combination for alpha " << g_best_alpha - 1 << " and below was checked and found impossible.\n";
    } else {
        std::cout << "No solution better than " << initial_target << " was found.\n";
    }
    
    std::cout << "Total Time: " << diff.count() << " seconds\n";
    std::cout << std::string(50, '=') << std::endl;

    return 0;
}