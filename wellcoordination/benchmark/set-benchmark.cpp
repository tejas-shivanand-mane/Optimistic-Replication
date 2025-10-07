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

#include "project.hpp"

std::string getRandomElement(const std::unordered_set<std::string> &set)
{
  std::vector<std::string> elements(set.begin(), set.end());
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, elements.size() - 1);
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

  std::string loc = "/home/jsabe004/Optimistic-Replication/wellcoordination/workload/";

  int nr_procs = std::atoi(argv[1]);
  int num_ops = std::atoi(argv[2]);
  double write_percentage = std::atof(argv[3]);

  loc += std::to_string(nr_procs) + "-" + std::to_string(num_ops) + "-" +
         std::to_string(static_cast<int>(write_percentage));
  loc += "/set/";

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

  int write_calls = total_writes;

  // Initialize C++11 random generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(1, 1000);

  for (int i = 1; i <= nr_procs; i++)
  {
    for (int type = 0; type <= 1; type++)
    {
      int count = 0;

      for (; count < expected_calls_per_update_method;)
      {
        std::string callStr;
        if (type == 0)
        {
          std::string c_id = std::to_string(dist(gen));
          callStr = "0 " + c_id;
          real_num_calls++;
        }
        else if (type == 1)
        {
          std::string c_id = std::to_string(dist(gen));
          callStr = "1 " + c_id;
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

  while (calls[0].size() > calls[1].size() && read_calls != 0)
  {
    calls[(index % (nr_procs - 1)) + 1].push_back(std::string("2"));
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
        calls[i].push_back(std::string("2"));

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
    for (int x = 0; x < calls[i].size(); x++)
      outfile[i] << calls[i][x] << std::endl;
    outfile[i].close();
  }

  std::cout << "expected calls to receive: " << write_calls << std::endl;
  std::cout << "real_num_calls: " << real_num_calls << std::endl;
  return 0;
}
