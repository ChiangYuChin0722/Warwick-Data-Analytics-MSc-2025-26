# CS917 Lab 1: Search Algorithms and Complexity

## (A) Implementation Summary
We implemented two sorting algorithms to reorder names by their priority value from the file `namePriorities.txt`.

- **Bubble Sort:**  
  A simple comparison-based algorithm that repeatedly swaps adjacent elements if they are in the wrong order.  
  → **Time Complexity:** O(n²)

- **Merge Sort:**  
  A divide-and-conquer recursive algorithm that splits the list into halves, sorts them separately, and merges them.  
  → **Time Complexity:** O(n log n)

Both algorithms were implemented manually (without using Python’s built-in `sort()`), and execution time was measured using `timeit.default_timer()`.

---

## (B) Benchmarking Results

Average over 10 runs (first 5000 records from `namePriorities.txt`):

| Algorithm     | Average Time (seconds) |
|---------------|-----------------------:|
| Bubble Sort   | 0.6267 |
| Merge Sort    | 0.0050 |

> **Observation:** Merge Sort is roughly 125× faster than Bubble Sort.

---

## (C) Explanation
Bubble Sort performs nested loops, comparing each element with every other element, leading to quadratic growth in operations (O(n²)).  
Merge Sort recursively splits the list and merges in linear time per level, resulting in O(n log n) total complexity.  
Thus, Merge Sort is much more efficient and scales better with large datasets.

---

## (D) Bonus Question
**Would it have been cheaper to store them sorted as priorities were assigned?**  
Yes — maintaining a sorted order during insertion (e.g., via a heap or ordered structure) only costs O(log n) per operation, which is far cheaper than sorting later in O(n log n) or O(n²).

---

## ✅ Conclusion
Merge Sort demonstrates dramatically better performance than Bubble Sort.  
For large datasets, using an efficient O(n log n) algorithm is essential for scalability and real-world applications.

---

## Appendix: Source Code

```python
import timeit
# (put your sorting.py code here)
