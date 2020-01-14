#ifndef HAVE_COMPRESSION_HPP
#define HAVE_COMPRESSION_HPP
// Autogenerated - do not edit by hand !
#define GLOBAL_HASH_TYPE unsigned int
#define GLOBAL_COMPRESS_TOL 16
#define GLOBAL_UNION_OR_STRUCT union
#define stringify(s) #s
#define xstringify(s) stringify(s)
#define THREAD_NUM 0
#define GLOBAL_INDEX_VECTOR std::vector<GLOBAL_INDEX_TYPE>
#define GLOBAL_INDEX_TYPE unsigned int
#define ASSERT2(x, msg)                          \
  if (!(x)) {                                    \
    Rcerr << "ASSERTION FAILED: " << #x << "\n"; \
    Rcerr << "POSSIBLE REASON: " << msg << "\n"; \
    abort();                                     \
  }
#define GLOBAL_MAX_NUM_THREADS 48
#define INDEX_OVERFLOW(x) \
  ((size_t)(x) >= (size_t)std::numeric_limits<GLOBAL_INDEX_TYPE>::max())
#define ASSERT(x)                                \
  if (!(x)) {                                    \
    Rcerr << "ASSERTION FAILED: " << #x << "\n"; \
    abort();                                     \
  }
#define GLOBAL_REPLAY_TYPE ad_aug
#define GLOBAL_MIN_PERIOD_REP 10
#define INHERIT_CTOR(A, B)                                       \
  A() {}                                                         \
  template <class T1>                                            \
  A(const T1 &x1) : B(x1) {}                                     \
  template <class T1, class T2>                                  \
  A(const T1 &x1, const T2 &x2) : B(x1, x2) {}                   \
  template <class T1, class T2, class T3>                        \
  A(const T1 &x1, const T2 &x2, const T3 &x3) : B(x1, x2, x3) {} \
  template <class T1, class T2, class T3, class T4>              \
  A(const T1 &x1, const T2 &x2, const T3 &x3, const T4 &x4)      \
      : B(x1, x2, x3, x4) {}
#define GLOBAL_SCALAR_TYPE double
#include "global.hpp"
#include "graph_transform.hpp"  // subset
#include "radix.hpp"            // first_occurance

namespace TMBad {

/** \brief Representation of a period in a sequence */
struct period {
  /** \brief Where does the period begin */
  size_t begin;
  /** \brief Size of the period */
  size_t size;
  /** \brief Number of consecutive period replicates */
  size_t rep;
};

std::ostream &operator<<(std::ostream &os, const period &x);

/** \brief Period analyzer

    Finds periodic patterns in sequences, e.g. the operation stack or
    the operator input sequence. The complexity is linear in the
    sequence length and in the maximum period size.

    ## Example 1:

    Periods with high degree of compression are selected. In the
    example below the period `1 2 1 2 3` with replicator 3 is
    preferred over `1 2` with replicator 2.

    ```
    {1, 2, 1, 2, 3, 1, 2, 1, 2, 3, 1, 2, 1, 2, 3, 1, 1, 1}
    begin: 0 size: 5 rep: 3
    begin: 15 size: 1 rep: 3
    ```
*/
template <class T>
struct periodic {
  const std::vector<T> &x;
  /** \brief Test periods up to this size */
  size_t max_period_size;
  /** \brief Ignore periods with too small replicator */
  size_t min_period_rep;
  periodic(const std::vector<T> &x, size_t max_period_size,
           size_t min_period_rep = 2)
      : x(x),
        max_period_size(max_period_size),
        min_period_rep(min_period_rep) {}
  /** \brief Test period one step ahead

      Tests if `all(x[i]==x[i+p])` where `i = start + 0:(p-1)`.
  */
  bool test_period(size_t start, size_t p) {
    if (start + (p - 1) + p >= x.size()) return false;
    for (size_t i = 0; i < p; i++) {
      if (x[start + i] != x[start + i + p]) return false;
    }
    return true;
  }
  /** \brief Find number period replicates

      Find number of consecutive period replicates starting from a given index
     `start`.
  */
  size_t numrep_period(size_t start, size_t p) {
    size_t n = 1;
    while (test_period(start, p)) {
      n++;
      start += p;
    }
    return n;
  }
  /** \brief Find the best period (highest compression degree) starting from a
     given index.

      A period of size `p` with replicator `r` reduces the full vector
      representation `p*r` to just `p`, i.e. relative degree of
      compression is `r`. Starting form `start` we thus seek the
      period with highest replicator.
      The function tests all periods up to a specified `max_period_size`.
      Note that, once a period `p` with replicator `r` has been found,
      it is redundant to test periods shorter than `p*r+1`.
  */
  period find_best_period(size_t start) {
    size_t p_best = -1, rep_best = 0;
    for (size_t p = 1; p < max_period_size; p++) {
      size_t rep = numrep_period(start, p);
      if (rep > rep_best) {
        p_best = p;
        rep_best = rep;
        p = p * rep;
      }
    }
    period ans = {start, p_best, rep_best};
    return ans;
  }
  std::vector<period> find_all() {
    std::vector<period> ans;
    for (size_t i = 0; i < x.size();) {
      period result = find_best_period(i);
      if (result.rep >= min_period_rep) {
        ans.push_back(result);
        i += result.size * result.rep;
      } else {
        i++;
      }
    }
    return ans;
  }
};

template <class T>
struct matrix_view {
  const T *x;
  size_t nrow, ncol;
  matrix_view(const T *x, size_t nrow, size_t ncol)
      : x(x), nrow(nrow), ncol(ncol) {}
  T operator()(size_t i, size_t j) const { return x[i + j * nrow]; }
  size_t rows() const { return nrow; }
  size_t cols() const { return ncol; }
  template <class Diff_T>
  std::vector<Diff_T> row_diff(size_t i) {
    size_t nd = (cols() >= 1 ? cols() - 1 : 0);
    std::vector<Diff_T> xd(nd);
    for (size_t j = 1; j < cols(); j++)
      xd[j - 1] = (Diff_T)(*this)(i, j) - (Diff_T)(*this)(i, j - 1);
    return xd;
  }
};

/** \brief Helper
    - Given a operator period, i.e. (begin, size, rep)
    - Given the full input sequence (glob.inputs)
    Make a split of the operator period such that the inputs are periodic as
   well. The split is represented as a vector of periods. Algorithm:
    1. Find matrix of input diffs
    2. Row-wise, find all periods and mark the 'period boundaries', i.e. (begin,
   begin+size*rep) in a common boolean workspace (shared by all rows).
    3. Construct the period split. Each mark signifies the beginning of a new
   period.

    FIXME: This function implementation is still a bit unclear
*/
std::vector<period> split_period(global *glob, period p,
                                 size_t max_period_size);

struct compressed_input {
  typedef std::ptrdiff_t ptrdiff_t;

  mutable std::vector<ptrdiff_t> increment_pattern;
  std::vector<Index> which_periodic;
  std::vector<Index> period_sizes;
  std::vector<Index> period_offsets;
  std::vector<ptrdiff_t> period_data;

  Index n, m;
  Index nrep;
  Index np;

  mutable Index counter;
  mutable std::vector<Index> inputs;
  std::vector<Index> input_diff;
  size_t input_size() const;
  void update_increment_pattern() const;

  void increment(Args<> &args) const;

  void decrement(Args<> &args) const;
  void forward_init(Args<> &args) const;
  void reverse_init(Args<> &args);
  void dependencies_intervals(Args<> &args, std::vector<Index> &lower,
                              std::vector<Index> &upper) const;

  size_t max_period_size;

  bool test_period(std::vector<ptrdiff_t> &x, size_t p);

  size_t find_shortest(std::vector<ptrdiff_t> &x);
  compressed_input();
  compressed_input(std::vector<Index> &x, size_t offset, size_t nrow, size_t m,
                   size_t ncol, size_t max_period_size);
};

template <class T1, class T2>
struct compare_types {
  const static bool equal = false;
};
template <class T>
struct compare_types<T, T> {
  const static bool equal = true;
};

void compress(global &glob, size_t max_period_size);
struct StackOp : global::SharedDynamicOperator {
  typedef std::ptrdiff_t ptrdiff_t;
  global::operation_stack opstack;
  compressed_input ci;
  StackOp(global *glob, period p, IndexPair ptr, size_t max_period_size);
  /** \brief Copy StackOp */
  StackOp(const StackOp &x);
  void print(global::print_config cfg);
  Index input_size() const;
  Index output_size() const;
  static const bool have_input_size_output_size = true;
  /** \brief Forward method applicable for Scalar and bool and Replay case
      \note In the replay case a tape compression is triggered after
      all individual operations have been replayed (decompressed). It
      follows, that only one StackOp can be in a decompressed state at
      a time.
  */
  template <class Type>
  void forward(ForwardArgs<Type> args) {
    ci.forward_init(args);

    size_t opstack_size = opstack.size();
    for (size_t i = 0; i < ci.nrep; i++) {
      for (size_t j = 0; j < opstack_size; j++) {
        opstack[j]->forward_incr(args);
      }
      ci.increment(args);
    }
    if (compare_types<Type, Replay>::equal) {
      compress(*get_glob(), ci.max_period_size);
    }
  }
  void forward(ForwardArgs<Writer> &args);
  /** \brief Reverse method applicable for Scalar and bool and Replay case
      \note In the replay case a tape compression is triggered after
      all individual operations have been replayed (decompressed). It
      follows, that only one StackOp can be in a decompressed state at
      a time.
  */
  template <class Type>
  void reverse(ReverseArgs<Type> args) {
    ci.reverse_init(args);
    size_t opstack_size = opstack.size();
    for (size_t i = 0; i < ci.nrep; i++) {
      ci.decrement(args);

      for (size_t j = opstack_size; j > 0;) {
        j--;
        opstack[j]->reverse_decr(args);
      }
    }
    if (compare_types<Type, Replay>::equal) {
      compress(*get_glob(), ci.max_period_size);
    }
  }
  void reverse(ReverseArgs<Writer> &args);
  /** \brief Dependencies (including indirect)
      \details StackOp assumes a certain memory layout
      and therefore have *indirect dependencies*.
  */
  void dependencies(Args<> args, Dependencies &dep) const;
  /** \brief `dependencies` has been implemented */
  static const bool have_dependencies = true;
  /** \brief Add marker routines based on `dependencies` */
  static const bool implicit_dependencies = true;
  /** \brief It is **not* safe to remap the inputs of this operator */
  static const bool allow_remap = false;
  const char *op_name();
};

template <class T>
void trim(std::vector<T> &v, const T &elt) {
  v.erase(std::remove(v.begin(), v.end(), elt), v.end());
}

template <class T>
struct toposort_remap {
  std::vector<T> &remap;
  T i;
  toposort_remap(std::vector<T> &remap, T i) : remap(remap), i(i) {}
  void operator()(Index k) {
    if (remap[k] >= remap[i]) {
      remap[i] = i;
    }
  }
};

/** \brief Re-order computational graph to make it more compressible

    \details The degree of compression offered by the periodic
    sequence analysis can be greatly improved by re-ordering the
    computational graph before the analysis.
    The re-ordering algorithm works as follows (see also
   `remap_identical_sub_expressions`).

    1. **Hash step** Two sub-expressions are considered 'weakly
    similar' if they are result of the same operator sequence without
    necessarily sharing any constants or independent variables. Assume
    we have a vector of hash codes 'h' - one for each
    sub-expression. Expressions with the same hash code are likely
    'weakly similar'. Now `remap:=first_occurance(h)` gives a likely
    valid reordering of variables satisfying `remap[i] <= i`.

    2. **Proof step** Assume by induction that `remap[j]` is valid for
    all j strictly less than i. Test if remap[i] is also valid
    (i.e. max(remap[dependencies(v2o(i))]) < remap[i]). If yes keep
    it. If no reject it be setting remap[i]=i (which is always valid).

    3. Now `remap` provides a valid ordering of the graph using the
    permutation `radix::order(remap)`
*/
void reorder_sub_expressions(global &glob);

void compress(global &glob, size_t max_period_size = 1024);

}  // namespace TMBad
#endif  // HAVE_COMPRESSION_HPP
