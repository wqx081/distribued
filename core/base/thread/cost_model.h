#ifndef CORE_BASE_THREAD_COST_MODEL_H_
#define CORE_BASE_THREAD_COST_MODEL_H_

#include <algorithm>
#include <iostream>

namespace eigen {

class OpCost {
 public:
  OpCost() : bytes_loaded_(0), bytes_stored_(0), compute_cycles_(0) {}

  OpCost(double bytes_loaded, double bytes_stored, double compute_cycles)
    : bytes_loaded_(bytes_loaded),
      bytes_stored_(bytes_stored),
      compute_cycles_(compute_cycles) {
  }

  inline double bytes_loaded() const {
      return bytes_loaded_;
  }
  inline double bytes_stored() const {
      return bytes_stored_;
  }
  inline double compute_cycles() const {
      return compute_cycles_;
  }
  inline double total_cost(
        double load_cost, double store_cost, double compute_cost) const {
      return load_cost * bytes_loaded_ + store_cost * bytes_stored_ +
             compute_cost * compute_cycles_;
  }

  friend std::ostream& operator<<(std::ostream& os, const OpCost& tc) {
      return os << "[bytes_loaded = " << tc.bytes_loaded()
                << ", bytes_stored = " << tc.bytes_stored()
                << ", compute_cycles = " << tc.compute_cycles() << "]";
  }

 private:
  double bytes_loaded_;
  double bytes_stored_;
  double compute_cycles_;
};

template<typename Device>
class CostModel {
 public:
  static const int kDeviceCyclesPerComputeCycle = 1;

  static const int kStartupCycles = 100000;
  static const int kPerThreadCycles = 100000;
  static const int kTaskSize = 40000;


  static inline int numThreads(
    double output_size, const OpCost& cost_per_coeff, int max_threads) {
      double cost = totalCost(output_size, cost_per_coeff);
      int threads = (cost - kStartupCycles) / kPerThreadCycles + 0.9;
      return std::min(max_threads, std::max(1, threads));
  }

  static inline double taskSize(
        double output_size, const OpCost& cost_per_coeff) {
    return totalCost(output_size, cost_per_coeff) / kTaskSize;
  }
  
 private:
  static double totalCost(double output_size, const OpCost& cost_per_coeff) {
    const double kLoadCycles = 1.0 / 64 * 11;
    const double kStoreCycles = 1.0 / 64 * 11;
    return output_size *
          cost_per_coeff.total_cost(kLoadCycles, kStoreCycles,
                                    kDeviceCyclesPerComputeCycle);
  }        
};

} // namespace eigen
#endif // CORE_BASE_THREAD_COST_MODEL_H_
