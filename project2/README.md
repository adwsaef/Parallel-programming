# Disjoint Multisets Problem

A multiset A is called `d-bounded`, for a natural number d, if it is finite and all its elements belong to {1, ..., d} (with arbitrary repetitions).

---

A pair of d-bounded multisets A, B is called `disjoint` if for all
A' subset of A and B' subset of B we have:

sum(A') = sum(B')  if and only if  A' = B' = empty set  or  (A' = A and B' = B)

In other words:

- sum(A) = sum(B), and
- The sums of any nonempty submultisets of A and B must differ.

---

## Problem
For a fixed d >= 3 (smaller d are not considered) and multisets A0, B0, we want to find disjoint d-bounded multisets A containing A0 and B containing B0 that maximize the value:

sum(A)  (equivalently sum(B))

We denote this value by alpha(d, A0, B0).

We set alpha(d, A0, B0) = 0 if:

- A0 and B0 are not d-bounded, or
- They do not admit d-bounded disjoint supermultisets.

## Project

We are provided with a reference solution to the problem, which performs a naive calculation.

The goal is to implement **two additional versions** of this solution: a **non-recursive version** and a **parallel version**. Both versions should perform the same calculations as the reference solution, using the same functions (e.g., `get_sumset_intersection_size`, `sumset_add`, etc.), and each function must be called **at least as many times as in the reference**. The implementations should be optimized for speed.

The file `report.pdf` contains scalability data for the parallel solution.

## Input

The first line contains four integers: `t`, `d`, `n`, `m`.

- `t` — the number of threads available (the main thread should not be counted if it does not perform any calculations).  
- `d` — parameter of the problem.  
- `n`, `m` — the sizes of sets A and B, respectively.

```
8 10 0 1

1
```