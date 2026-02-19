#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <algorithm>
#include <random>
#include <vector>
#include <unordered_set>
#include <cmath>

#include "project.hpp"

/* ---------------- Zipfian ---------------- */
class ZipfianGenerator {
  uint64_t items;
  double theta, zetan, alpha, eta;
  std::mt19937_64 gen;
  std::uniform_real_distribution<double> dist;

  static double zeta(uint64_t n, double theta) {
    double sum = 0.0;
    for (uint64_t i = 1; i <= n; i++)
      sum += 1.0 / std::pow((double)i, theta);
    return sum;
  }

public:
  ZipfianGenerator(uint64_t n, double theta = 0.99)
      : items(n), theta(theta),
        gen(std::random_device{}()), dist(0.0, 1.0) {

    zetan = zeta(items, theta);
    alpha = 1.0 / (1.0 - theta);
    double zeta2 = zeta(2, theta);
    eta = (1 - std::pow(2.0 / (double)items, 1 - theta)) /
          (1 - zeta2 / zetan);
  }

  uint64_t next() {
    double u = dist(gen);
    double uz = u * zetan;
    if (uz < 1.0) return 0;
    if (uz < 1.0 + std::pow(0.5, theta)) return 1;
    return static_cast<uint64_t>(
        (double)items * std::pow(eta * u - eta + 1, alpha));
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

std::string getRandomElement(const std::unordered_set<std::string> &set)
{
  std::vector<std::string> elements(set.begin(), set.end());
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, (int)elements.size() - 1);
  int randomIndex = dis(gen);
  return elements[randomIndex];
}

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
  double write_percentage = std::atof(argv[3]);

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

  write_percentage /= 100;
  double total_writes = num_ops * write_percentage;
  int queries = num_ops - total_writes;

  int num_conflicting_write_methods = 0;
  int num_nonconflicting_write_methods = 2;
  int num_read_methods = 1;
  int real_num_calls = 0;

  std::cout << "ops: " << num_ops << std::endl;
  std::cout << "write_perc: " << write_percentage << std::endl;
  std::cout << "writes: " << total_writes << std::endl;
  std::cout << "reads: " << queries << std::endl;

  int expected_nonconflicting_write_calls_per_node = total_writes / (nr_procs);
  int expected_calls_per_update_method = expected_nonconflicting_write_calls_per_node / num_nonconflicting_write_methods;

  std::cout << "expected #calls per update method "
            << expected_calls_per_update_method << std::endl;

  int write_calls = (int)total_writes;

  /* ------------------------------------------------------------
   * YCSB parameters (key generation)
   * ------------------------------------------------------------ */
  constexpr uint64_t KEYSPACE = 1000;
  ScrambledZipfianGenerator keygen(KEYSPACE);

  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<int> write_op_dist(0, 1); // 0 or 1

  for (int i = 1; i <= nr_procs; i++)
  {
    for (int type = 0; type <= 1; type++)
    {
      int count = 0;

      for (; count < expected_calls_per_update_method;)
      {
        std::string callStr;

        // YCSB-like scrambled-zipfian key
        uint64_t key = keygen.next();

        if (type == 0)
        {
          callStr = "0 " + std::to_string(key);
          real_num_calls++;
        }
        else if (type == 1)
        {
          callStr = "1 " + std::to_string(key);
          real_num_calls++;
        }

        calls[i - 1].push_back(callStr);
        count++;
      }
    }
  }

  int q = num_ops - write_calls;
  std::cout << "q: " << q << std::endl;

  int read_calls = q;
  int index = 0;

  std::cout << "after adding writes nodes" << std::endl;
  for (int i = 0; i < nr_procs; i++)
    std::cout << i + 1 << " size: " << calls[i].size() << std::endl;

  while (nr_procs >= 2 && calls[0].size() > calls[1].size() && read_calls != 0)
  {
    uint64_t key = keygen.next();
    calls[(index % (nr_procs - 1)) + 1].push_back(std::string("2 " + std::to_string(key)));
    read_calls--;
    index++;
  }

  std::cout << "after adding reads" << std::endl;
  for (int i = 0; i < nr_procs; i++)
    std::cout << i + 1 << " size: " << calls[i].size() << std::endl;

  if (read_calls != 0)
  {
    for (int i = 0; i < nr_procs; i++)
      for (int j = 0; j < read_calls / nr_procs; j++)
      {
        uint64_t key = keygen.next();
        calls[i].push_back(std::string("2"));
      }

    std::cout << "after adding reads to all" << std::endl;
    for (int i = 0; i < nr_procs; i++)
      std::cout << i + 1 << " size: " << calls[i].size() << std::endl;
  }

  for (int i = 0; i < nr_procs; i++)
  {
    calls[i].insert(calls[i].begin(), std::string("#" + std::to_string(real_num_calls)));
    std::random_shuffle(calls[i].begin() + 1, calls[i].end());
  }

  for (int i = 0; i < nr_procs; i++)
  {
    for (int x = 0; x < (int)calls[i].size(); x++)
      outfile[i] << calls[i][x] << std::endl;
    outfile[i].close();
  }

  std::cout << "expected calls to receive: " << write_calls << std::endl;
  std::cout << "real_num_calls: " << real_num_calls << std::endl;
  return 0;
}
