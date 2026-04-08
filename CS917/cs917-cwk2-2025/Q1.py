# q1_growth_test.py
# --------------------------------------------
# CS917 Coursework Question 1
# Estimating the largest n for various time complexities
# Author: Your Name / Student ID
# --------------------------------------------

import math

# --------------------------------------------
# Define time limits (in microseconds)
# --------------------------------------------
times = {
    "1 second": 1e6,
    "1 minute": 60 * 1e6,
    "1 hour": 3600 * 1e6,
    "1 day": 86400 * 1e6,
    "1 month": 30 * 86400 * 1e6,
    "1 year": 365 * 86400 * 1e6
}

# --------------------------------------------
# Define common complexity functions
# --------------------------------------------
funcs = {
    "n log2 n": lambda n: n * math.log2(n) if n > 0 else 0,
    "n^2": lambda n: n ** 2,
    "n^3": lambda n: n ** 3,
    "2^n": lambda n: 2 ** n,
    "n!": lambda n: math.factorial(n)
}

# --------------------------------------------
# Function to find the largest n satisfying f(n) <= t
# Add a safety limit to avoid infinite loops
# --------------------------------------------
def max_n(f, t, low=1, high=None):
    """Find largest n such that f(n) <= t, using binary search."""
    # Step 1: 找上界（倍增法）
    n = 1
    while high is None:
        try:
            if f(n) > t:
                high = n
                break
            n *= 2
        except (OverflowError, ValueError):
            high = n
            break

    low = n // 2
    # Step 2: 二分搜尋
    while low < high - 1:
        mid = (low + high) // 2
        try:
            if f(mid) <= t:
                low = mid
            else:
                high = mid
        except (OverflowError, ValueError):
            high = mid
    return low


# --------------------------------------------
# Run experiment
# --------------------------------------------
def main():
    print(f"{'Function':<10} | {'1s':>10} | {'1m':>10} | {'1h':>10} | {'1d':>10} | {'1mo':>10} | {'1y':>10}")
    print("-" * 80)

    for fname, f in funcs.items():
        results = []
        for _, t in times.items():
            n_val = max_n(f, t)
            results.append(f"{n_val:>10,.0f}")
        print(f"{fname:<10} | " + " | ".join(results))

if __name__ == "__main__":
    main()
