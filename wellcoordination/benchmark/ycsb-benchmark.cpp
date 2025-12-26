#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cmath>

#include "ycsb.hpp"

/* ---------------- Zipfian ---------------- */
class ZipfianGenerator {
  uint64_t items;
  double theta, zetan, alpha, eta;
  std::mt19937_64 gen;
  std::uniform_real_distribution<double> dist;

  static double zeta(uint64_t n, double theta) {
    double sum = 0.0;
    for (uint64_t i = 1; i <= n; i++)
      sum += 1.0 / std::pow(i, theta);
    return sum;
  }

public:
  ZipfianGenerator(uint64_t n, double theta = 0.99)
      : items(n), theta(theta),
        gen(std::random_device{}()), dist(0.0, 1.0) {

    zetan = zeta(items, theta);
    alpha = 1.0 / (1.0 - theta);
    double zeta2 = zeta(2, theta);
    eta = (1 - std::pow(2.0 / items, 1 - theta)) /
          (1 - zeta2 / zetan);
  }

  uint64_t next() {
    double u = dist(gen);
    double uz = u * zetan;
    if (uz < 1.0) return 0;
    if (uz < 1.0 + std::pow(0.5, theta)) return 1;
    return static_cast<uint64_t>(
        items * std::pow(eta * u - eta + 1, alpha));
  }
};

/* ---------------- Scrambled Zipfian ---------------- */
static inline uint64_t fnvhash64(uint64_t v) {
  const uint64_t FNV_OFFSET = 14695981039346656037ULL;
  const uint64_t FNV_PRIME  = 1099511628211ULL;
  uint64_t h = FNV_OFFSET;
  for (int i = 0; i < 8; i++) {
    h ^= (v & 0xff);
    h *= FNV_PRIME;
    v >>= 8;
  }
  return h;
}

class ScrambledZipfianGenerator {
  ZipfianGenerator zipf;
  uint64_t items;

public:
  ScrambledZipfianGenerator(uint64_t n)
      : zipf(n), items(n) {}

  uint64_t next() {
    return fnvhash64(zipf.next()) % items;
  }
};



int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Usage: " << argv[0] << " <nr_procs> <num_ops> <write_percentage>" << std::endl;
    return 1;
  }

  std::string loc = "/home/tejas/Optimistic-Replication/wellcoordination/workload/";

  int nr_procs = std::atoi(argv[1]);
  int num_ops = std::atoi(argv[2]);

  double write_percentage = std::atof(argv[3]) / 100.0;

  loc += std::to_string(nr_procs) + "-" + std::to_string(num_ops) + "-" +
         std::to_string(static_cast<int>(write_percentage));
  loc += "/ycsb/";



  std::vector<std::ofstream> outfile(nr_procs);
  std::vector<std::vector<std::string>> calls(nr_procs);
  for (int i = 0; i < nr_procs; i++)
  {
    remove((loc + std::to_string(i + 1) + ".txt").c_str());
    outfile[i].open(loc + std::to_string(i + 1) + ".txt", std::ios_base::app);
    if (!outfile[i].is_open())
    {
      std::cerr << "Failed to open file: " << loc + std::to_string(i + 1) + ".txt" << std::endl;
      return 1;
    }
    calls[i] = std::vector<std::string>();
  }


/* ------------------------------------------------------------
   * YCSB parameters
   * ------------------------------------------------------------ */
  constexpr uint64_t KEYSPACE = 1'000'000;

  ScrambledZipfianGenerator keygen(KEYSPACE);

  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::bernoulli_distribution is_write(write_percentage);

  /* ------------------------------------------------------------
   * Generate YCSB operations
   * ------------------------------------------------------------ */
  for (int i = 0; i < nr_procs; i++) {
    for (int j = 0; j < num_ops / nr_procs; j++) {

      uint64_t key = keygen.next();

      if (is_write(gen)) {
        // UPDATE
        calls[i].push_back(
            "0 " + std::to_string(key));
      } else {
        // READ
        calls[i].push_back(
            "2 " + std::to_string(key));
      }
    }
  }

  /* ------------------------------------------------------------
   * Write files
   * ------------------------------------------------------------ */
  for (int i = 0; i < nr_procs; i++) {
    outfile[i] << "#" << calls[i].size() << "\n";
    for (const auto &c : calls[i])
      outfile[i] << c << "\n";
    outfile[i].close();
  }

  std::cout << "Generated YCSB workload\n";
  std::cout << "Processes: " << nr_procs << "\n";
  std::cout << "Operations: " << num_ops << "\n";
  std::cout << "Write ratio: " << write_percentage << "\n";


  return 0;
}
