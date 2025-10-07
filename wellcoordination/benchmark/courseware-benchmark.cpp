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
  loc += "/courseware/";

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

  int num_nonconflicting_write_methods = 5;
  int real_num_calls = 0;

  std::cout << "ops: " << num_ops << std::endl;
  std::cout << "write_perc: " << write_percentage << std::endl;
  std::cout << "writes: " << total_writes << std::endl;
  std::cout << "reads: " << queries << std::endl;

  int expected_nonconflicting_write_calls_per_node = total_writes / nr_procs;
  int expected_calls_per_update_method = expected_nonconflicting_write_calls_per_node / num_nonconflicting_write_methods;

  std::cout << "expected #calls per update method "
            << expected_calls_per_update_method << std::endl;

  int write_calls = total_writes;

  std::string callStraddp, callStradde;
  std::vector<std::unordered_set<std::string>> addcourse(nr_procs), addstudent(nr_procs);
  callStraddp = std::string("0 ") + "1";
  callStradde = std::string("3 ") + "1";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> rand_1000(1, 1000);

  for (int i = 1; i <= nr_procs; i++)
  {
    addcourse[i - 1].insert(std::to_string(1000 * i));
    real_num_calls++;
    addstudent[i - 1].insert("1");
    real_num_calls++;

    for (int type = 0; type <= 4; type++)
    {
      int count = (type == 0 || type == 3) ? 1 : 0;

      for (; count < expected_calls_per_update_method;)
      {
        std::string callStr;
        if (type == 0)
        {
          std::string c_id = std::to_string((1000 * i) + rand_1000(gen));
          callStr = "0 " + c_id;
        }
        else if (type == 1)
        {
          std::string c_id = std::to_string((1000 * i) + 2 + rand_1000(gen));
          callStr = "1 " + c_id;
        }
        else if (type == 2)
        {
          std::string e_id = std::to_string(rand_1000(gen));
          std::string p_id = std::to_string(rand_1000(gen));
          callStr = "2 " + e_id + "-" + p_id;
        }
        else if (type == 3)
        {
          std::string s_id = std::to_string(rand_1000(gen));
          callStr = "3 " + s_id;
        }
        else if (type == 4)
        {
          std::string s_id = std::to_string(rand_1000(gen));
          callStr = "4 " + s_id;
        }
        real_num_calls++;
        calls[i - 1].push_back(callStr);
        count++;
      }
    }
  }

  int read_calls = queries;
  int index = 0;

  while (calls[0].size() > calls[1].size() && read_calls != 0)
  {
    calls[(index % (nr_procs - 1)) + 1].push_back("4");
    read_calls--;
    index++;
  }

  if (read_calls != 0)
  {
    for (int i = 0; i < nr_procs; i++)
      for (int j = 0; j < read_calls / nr_procs; j++)
        calls[i].push_back("5");
  }

  for (int i = 0; i < nr_procs; i++)
  {
    calls[i].insert(calls[i].begin(), "#" + std::to_string(real_num_calls));
    calls[i].insert(calls[i].begin() + 1, "0 " + std::to_string(1000 * (i + 1)));
    calls[i].insert(calls[i].begin() + 2, callStradde);
    std::random_shuffle(calls[i].begin() + 3, calls[i].end());
  }

  // Update workson entries to use real inserted IDs
  std::string callStr;
  for (int i = 0; i < nr_procs; i++)
  {
    for (auto it = calls[i].begin() + 3; it != calls[i].end(); ++it)
    {
      std::string call = *it;
      if (call != "5")
      {
        if (call.substr(0, 2) == "0 ")
        {
          std::string c_id = call.substr(2);
          addcourse[i].insert(c_id);
        }
        else if (call.substr(0, 2) == "1 ")
        {
          std::string c_id = call.substr(2);
          addcourse[i].erase(c_id);
        }
        else if (call.substr(0, 2) == "2 ")
        {
          std::string e_id = getRandomElement(addstudent[i]);
          std::string p_id = getRandomElement(addcourse[i]);
          *it = "2 " + e_id + "-" + p_id;
        }
        else if (call.substr(0, 2) == "3 ")
        {
          std::string c_id = call.substr(2);
          addstudent[i].insert(c_id);
        }
        else if (call.substr(0, 2) == "4 ")
        {
          std::string c_id = call.substr(2);
          addstudent[i].erase(c_id);
        }
      }
    }
  }

  for (int i = 0; i < nr_procs; i++)
  {
    for (const std::string &c : calls[i])
      outfile[i] << c << std::endl;
    outfile[i].close();
  }

  std::cout << "expected calls to receive: " << write_calls << std::endl;
  std::cout << "real_num_calls: " << real_num_calls << std::endl;
  return 0;
}
