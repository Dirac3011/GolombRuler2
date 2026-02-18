# GolombRuler-2

# B2-Sequence Finder (Generalized Sidon Sets)

An optimized C++ solver designed to find the minimum span ($\alpha$) for a set of $N$ integers where no pairwise difference occurs more than twice ($\gamma = 2$). 

## Technical Breakdown

### The Pruning Logic
The algorithm at any given depth calculates the minimum possible distance required to place the remaining $N - k$ elements. 

If the current value plus this minimum distance exceeds the best $\alpha$ found so far, the entire branch is killed instantly:
`if (last_val + min_span_remaining >= g_best_alpha) return;`

### Bitwise Difference Tracking
Pairwise differences are tracked using two 128-bit registers:
1.  `seen_once`: Bits are toggled when a difference is encountered for the first time.
2.  `seen_twice`: Bits are toggled when a difference is encountered for the second time.
If a new candidate number generates a difference already present in `seen_twice`, it is rejected in a single clock cycle.


## Compilation
For maximum performance, always compile with the `-O3` flag:

