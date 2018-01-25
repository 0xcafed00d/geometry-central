#pragma once

#include "Eigen/Sparse"

// Suitesparse includes, as needed
#ifdef HAVE_SUITESPARSE
#include "geometrycentral/suitesparse_utilities.h"
#include <SuiteSparseQR.hpp>
#include <cholmod.h>
#endif


// This disables various safety chceks in linear algebra code and solvers
// #define GC_NLINALG_DEBUG

// Note: actual solvers implemented with explicit template instantiation in solvers.cpp

// Utilitiy typedef
namespace {
template <typename T>
using Vector = Eigen::Matrix<T, Eigen::Dynamic, 1>;
}

namespace geometrycentral {

// Utility solvers, which use the classes below
template <typename T>
Vector<T> smallestEigenvectorPositiveDefinite(Eigen::SparseMatrix<T>& energyMatrix, Eigen::SparseMatrix<T>& massMatrix,
                                              size_t nIterations = 50);

// Base class for all linear solvers
template <typename T>
class LinearSolver {

public:
  LinearSolver(const Eigen::SparseMatrix<T>& mat_) { mat = Eigen::SparseMatrix<T, Eigen::ColMajor>(mat_); }

  // Solve for a particular right hand side
  Vector<T> operator()(const Vector<T>& rhs);

  // Solve for a particular right hand side, and return in an existing vector objects
  virtual void operator()(Vector<T>& x, const Vector<T>& rhs) = 0;

  // Compute the residual of a solve
  double residual(const Vector<T>& lhs, const Vector<T>& rhs);

protected:
  Eigen::SparseMatrix<T, Eigen::ColMajor> mat;
};

// General solver (uses QR)
// Computes least-squares solution for overdetermined systems, minimum norm solution for underdetermined systems
// TODO name is dumb
template <typename T>
class Solver : public LinearSolver<T> {

public:
  Solver(const Eigen::SparseMatrix<T>& mat_) : LinearSolver<T>(mat_) { prepare(); }
  ~Solver();

#ifdef HAVE_SUITESPARSE
  // Type wizardry. This type is 'double' if T == 'float', and T otherwise
  typedef typename std::conditional<std::is_same<T, float>::value, double, T>::type SOLVER_ENTRYTYPE;
#endif

  // Solve!
  virtual void operator()(Vector<T>& x, const Vector<T>& rhs) override;

  // Static solve without retaining the factorization
  static Vector<T> solve(const Eigen::SparseMatrix<T>& A, const Vector<T>& rhs);
  static void solve(const Eigen::SparseMatrix<T>& A, Vector<T>& x, const Vector<T>& rhs);

  // Gets the rank of the system
  size_t rank();

protected:
  void prepare();

// Implementation-specific quantities
#ifdef HAVE_SUITESPARSE
  bool underdetermined;
  CholmodContext context;
  cholmod_sparse* cMat = nullptr;
  cholmod_sparse* cMatTrans = nullptr;
  SuiteSparseQR_factorization<typename Solver<T>::SOLVER_ENTRYTYPE>* factorization = nullptr;
  double zero_tolerance = -2; // (use default)
#else
  Eigen::SparseQR<Eigen::SparseMatrix<T>> solver;
#endif
};

template <typename T>
class PositiveDefiniteSolver : public LinearSolver<T> {

public:
  PositiveDefiniteSolver(const Eigen::SparseMatrix<T>& mat_) : LinearSolver<T>(mat_) { prepare(); }
  ~PositiveDefiniteSolver();

  // Solve!
  virtual void operator()(Vector<T>& x, const Vector<T>& rhs) override;

  // Static solve without retaining the factorization
  static Vector<T> solve(const Eigen::SparseMatrix<T>& A, const Vector<T>& rhs);
  static void solve(const Eigen::SparseMatrix<T>& A, Vector<T>& x, const Vector<T>& rhs);

protected:
  void prepare();

// Implementation-specific quantities
#ifdef HAVE_SUITESPARSE
  CholmodContext context;
  cholmod_sparse* cMat = nullptr;
  cholmod_factor* factorization = nullptr;
#else
  Eigen::SimplicialLDLT<Eigen::SparseMatrix<T>> solver;
#endif
};

template <typename T>
class SquareSolver : public LinearSolver<T> {

public:
  SquareSolver(const Eigen::SparseMatrix<T>& mat_) : LinearSolver<T>(mat_) { prepare(); }
  ~SquareSolver();

  // Solve!
  virtual void operator()(Vector<T>& x, const Vector<T>& rhs) override;

  // Static solve without retaining the factorization
  static Vector<T> solve(const Eigen::SparseMatrix<T>& A, const Vector<T>& rhs);
  static void solve(const Eigen::SparseMatrix<T>& A, Vector<T>& x, const Vector<T>& rhs);

protected:
  void prepare();

  // Implementation-specific quantities
#ifdef HAVE_SUITESPARSE
  CholmodContext context;
  cholmod_sparse* cMat = nullptr;
  void* symbolicFactorization = nullptr;
  void* numericFactorization = nullptr;
#else
  Eigen::SparseLU<Eigen::SparseMatrix<T>> solver;
#endif
};

} // namespace geometrycentral