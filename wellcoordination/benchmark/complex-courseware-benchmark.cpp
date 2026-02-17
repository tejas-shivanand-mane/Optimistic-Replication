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
#include <unordered_map>
#include <sstream>

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

  std::string loc = "/home/tejas/Optimistic-Replication/wellcoordination/workload/";

  int nr_procs = std::atoi(argv[1]);
  int num_ops = std::atoi(argv[2]);
  double write_percentage = std::atof(argv[3]);

  loc += std::to_string(nr_procs) + "-" + std::to_string(num_ops) + "-" +
         std::to_string(static_cast<int>(write_percentage));
  loc += "/complex-courseware/";

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

  // ComplexCourseWare: 0..18 are writes, 19 is Read
  int num_nonconflicting_write_methods = 19;
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

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> rand_1000(1, 1000);

  // Track existing IDs per node so we can rewrite ops to always be permissible
  std::vector<std::unordered_set<std::string>> departments(nr_procs), free_departments(nr_procs);
  std::vector<std::unordered_set<std::string>> students(nr_procs), instructors(nr_procs), courses(nr_procs);
  std::vector<std::unordered_set<std::string>> enroll_pairs(nr_procs), teach_pairs(nr_procs);

  // Track course_name uniqueness (string-based, because file uses ints but we store as strings)
  std::vector<std::unordered_set<std::string>> course_names(nr_procs);
  std::vector<std::unordered_map<std::string, std::string>> course_id_to_name(nr_procs);

  // Seed IDs per node (not shuffled)
  std::vector<std::string> seed_dept(nr_procs), seed_student(nr_procs), seed_instructor(nr_procs), seed_course(nr_procs);

  for (int i = 1; i <= nr_procs; i++)
  {
    seed_dept[i - 1] = std::to_string(1000 * i + 1);
    seed_student[i - 1] = std::to_string(2000 * i + 1);
    seed_instructor[i - 1] = std::to_string(3000 * i + 1);
    seed_course[i - 1] = std::to_string(4000 * i + 1);

    departments[i - 1].insert(seed_dept[i - 1]);
    free_departments[i - 1].insert(seed_dept[i - 1]);
    real_num_calls++;

    students[i - 1].insert(seed_student[i - 1]);
    real_num_calls++;

    instructors[i - 1].insert(seed_instructor[i - 1]);
    real_num_calls++;

    courses[i - 1].insert(seed_course[i - 1]);
    course_names[i - 1].insert(seed_course[i - 1]); // we use course_name = course_id for uniqueness
    course_id_to_name[i - 1][seed_course[i - 1]] = seed_course[i - 1];
    real_num_calls++;

    // Mark seed_dept as used by the seeded instructor/course later in the file
    free_departments[i - 1].erase(seed_dept[i - 1]);

    for (int type = 0; type <= 18; type++)
    {
      // We seed: AddDepartment(13), AddStudent(0), AddInstructor(4), AddCourseForce(8)
      int count = (type == 13 || type == 0 || type == 4 || type == 8) ? 1 : 0;

      for (; count < expected_calls_per_update_method;)
      {
        std::string callStr;

        if (type == 0) // AddStudent: 0 <sid> <first> <last>
        {
          std::string s_id = std::to_string((2000 * i + 1) + rand_1000(gen));
          std::string first = std::to_string(rand_1000(gen));
          std::string last = std::to_string(rand_1000(gen));
          callStr = "0 " + s_id + " " + first + " " + last;
        }
        else if (type == 1) // DeleteStudentCascade: 1 <sid>  (fixup -> existing or no-op)
        {
          std::string s_id = std::to_string((2000 * i + 1) + rand_1000(gen));
          callStr = "1 " + s_id;
        }
        else if (type == 2) // UpdateStudentIDCascade: 2 <old>-<new> (fixup)
        {
          callStr = "2 0-0";
        }
        else if (type == 3) // UpdateStudentOther: 3 <sid> <first> <last> (fixup sid)
        {
          std::string s_id = std::to_string((2000 * i + 1) + rand_1000(gen));
          std::string first = std::to_string(rand_1000(gen));
          std::string last = std::to_string(rand_1000(gen));
          callStr = "3 " + s_id + " " + first + " " + last;
        }
        else if (type == 4) // AddInstructor: 4 <iid> <first> <last> <dept> (fixup dept)
        {
          std::string ins_id = std::to_string((3000 * i + 1) + rand_1000(gen));
          std::string first = std::to_string(rand_1000(gen));
          std::string last = std::to_string(rand_1000(gen));
          std::string dept = seed_dept[i - 1];
          callStr = "4 " + ins_id + " " + first + " " + last + " " + dept;
        }
        else if (type == 5) // DeleteInstructorCascade: 5 <iid> (fixup iid)
        {
          std::string ins_id = std::to_string((3000 * i + 1) + rand_1000(gen));
          callStr = "5 " + ins_id;
        }
        else if (type == 6) // UpdateInstructorIDCascade: 6 <old>-<new> (fixup)
        {
          callStr = "6 0-0";
        }
        else if (type == 7) // UpdateInstructorOther: 7 <iid> <first> <last> <dept> (fixup iid/dept)
        {
          std::string ins_id = std::to_string((3000 * i + 1) + rand_1000(gen));
          std::string first = std::to_string(rand_1000(gen));
          std::string last = std::to_string(rand_1000(gen));
          std::string dept = seed_dept[i - 1];
          callStr = "7 " + ins_id + " " + first + " " + last + " " + dept;
        }
        else if (type == 8) // AddCourseForce: 8 <cid> <cname> <sem> <dept> (fixup dept; cname unique)
        {
          std::string c_id = std::to_string((4000 * i + 1) + rand_1000(gen));
          std::string c_name = c_id;
          std::string sem = std::to_string((rand_1000(gen) % 8) + 1);
          std::string dept = seed_dept[i - 1];
          callStr = "8 " + c_id + " " + c_name + " " + sem + " " + dept;
        }
        else if (type == 9) // DeleteCourseCascade: 9 <cid> (fixup cid)
        {
          std::string c_id = std::to_string((4000 * i + 1) + rand_1000(gen));
          callStr = "9 " + c_id;
        }
        else if (type == 10) // UpdateCourseIDCascade: 10 <old>-<new> (fixup)
        {
          callStr = "10 0-0";
        }
        else if (type == 11) // UpdateCourseNameForce: 11 <cid> <newname> (fixup cid/newname)
        {
          std::string c_id = std::to_string((4000 * i + 1) + rand_1000(gen));
          std::string newname = std::to_string(900000 + rand_1000(gen));
          callStr = "11 " + c_id + " " + newname;
        }
        else if (type == 12) // UpdateCourseOther: 12 <cid> <sem> <dept> (fixup cid/dept)
        {
          std::string c_id = std::to_string((4000 * i + 1) + rand_1000(gen));
          std::string sem = std::to_string((rand_1000(gen) % 8) + 1);
          std::string dept = seed_dept[i - 1];
          callStr = "12 " + c_id + " " + sem + " " + dept;
        }
        else if (type == 13) // AddDepartment: 13 <dept>
        {
          std::string d_id = std::to_string((1000 * i + 1) + rand_1000(gen));
          callStr = "13 " + d_id;
        }
        else if (type == 14) // DeleteDepartmentNoAction: 14 <dept> (fixup to free or non-existent)
        {
          callStr = "14 0";
        }
        else if (type == 15) // Enroll: 15 <cid>-<sid> (fixup)
        {
          callStr = "15 0-0";
        }
        else if (type == 16) // WithdrawEnrollment: 16 <cid>-<sid> (fixup to existing pair if possible)
        {
          callStr = "16 0-0";
        }
        else if (type == 17) // Teach: 17 <cid>-<iid> (fixup)
        {
          callStr = "17 0-0";
        }
        else if (type == 18) // WithdrawTeaching: 18 <cid>-<iid> (fixup to existing pair if possible)
        {
          callStr = "18 0-0";
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
    calls[(index % (nr_procs - 1)) + 1].push_back("19");
    read_calls--;
    index++;
  }

  if (read_calls != 0)
  {
    for (int i = 0; i < nr_procs; i++)
      for (int j = 0; j < read_calls / nr_procs; j++)
        calls[i].push_back("19");
  }

  for (int i = 0; i < nr_procs; i++)
  {
    calls[i].insert(calls[i].begin(), "#" + std::to_string(real_num_calls));

    // Seeds (not shuffled)
    calls[i].insert(calls[i].begin() + 1, "13 " + seed_dept[i]); // AddDepartment

    // AddStudent (0): 0 <sid> <first> <last>
    calls[i].insert(calls[i].begin() + 2, "0 " + seed_student[i] + " " + seed_student[i] + " " + seed_student[i]);

    // AddInstructor (4): 4 <iid> <first> <last> <dept>
    calls[i].insert(calls[i].begin() + 3, "4 " + seed_instructor[i] + " " + seed_instructor[i] + " " + seed_instructor[i] + " " + seed_dept[i]);

    // AddCourseForce (8): 8 <cid> <cname> <sem> <dept>
    calls[i].insert(calls[i].begin() + 4, "8 " + seed_course[i] + " " + seed_course[i] + " 1 " + seed_dept[i]);

    std::random_shuffle(calls[i].begin() + 5, calls[i].end());
  }

  // Fixup pass: rewrite ops so locallyPermissibility always returns true in this final order
  for (int i = 0; i < nr_procs; i++)
  {
    for (auto it = calls[i].begin() + 5; it != calls[i].end(); ++it)
    {
      std::string call = *it;
      if (call != "19")
      {
        std::istringstream iss(call);
        int op = -1;
        iss >> op;

        // Helper lambdas (keep local; minimal changes)
        auto pickFreshId = [&](const std::unordered_set<std::string> &used, int base) -> std::string {
          std::string x;
          do
          {
            x = std::to_string(base + rand_1000(gen));
          } while (used.find(x) != used.end());
          return x;
        };

        auto erasePairsWithSuffix = [&](std::unordered_set<std::string> &pairs, const std::string &suffix) {
          for (auto pit = pairs.begin(); pit != pairs.end();)
          {
            std::string p = *pit;
            size_t dash = p.find('-');
            if (dash != std::string::npos)
            {
              std::string right = p.substr(dash + 1);
              if (right == suffix)
                pit = pairs.erase(pit);
              else
                ++pit;
            }
            else
              ++pit;
          }
        };

        auto erasePairsWithPrefix = [&](std::unordered_set<std::string> &pairs, const std::string &prefix) {
          for (auto pit = pairs.begin(); pit != pairs.end();)
          {
            std::string p = *pit;
            size_t dash = p.find('-');
            if (dash != std::string::npos)
            {
              std::string left = p.substr(0, dash);
              if (left == prefix)
                pit = pairs.erase(pit);
              else
                ++pit;
            }
            else
              ++pit;
          }
        };

        auto replacePairsPrefix = [&](std::unordered_set<std::string> &pairs, const std::string &oldp, const std::string &newp) {
          std::unordered_set<std::string> out;
          for (const auto &p : pairs)
          {
            size_t dash = p.find('-');
            if (dash != std::string::npos)
            {
              std::string left = p.substr(0, dash);
              std::string right = p.substr(dash + 1);
              if (left == oldp)
                out.insert(newp + "-" + right);
              else
                out.insert(p);
            }
          }
          pairs.swap(out);
        };

        auto replacePairsSuffix = [&](std::unordered_set<std::string> &pairs, const std::string &olds, const std::string &news) {
          std::unordered_set<std::string> out;
          for (const auto &p : pairs)
          {
            size_t dash = p.find('-');
            if (dash != std::string::npos)
            {
              std::string left = p.substr(0, dash);
              std::string right = p.substr(dash + 1);
              if (right == olds)
                out.insert(left + "-" + news);
              else
                out.insert(p);
            }
          }
          pairs.swap(out);
        };

        // 13 <dept>
        if (op == 13)
        {
          std::string dept_id;
          iss >> dept_id;
          if (!dept_id.empty())
          {
            departments[i].insert(dept_id);
            free_departments[i].insert(dept_id);
          }
        }
        // 0 <student_id> <first> <last>
        else if (op == 0)
        {
          std::string s_id;
          iss >> s_id;
          if (!s_id.empty())
            students[i].insert(s_id);
        }
        // 1 <student_id> (delete cascade) -> keep at least 1 student for Enroll
        else if (op == 1)
        {
          if (students[i].size() > 1)
          {
            std::string s_id = getRandomElement(students[i]);
            if (s_id == seed_student[i] && students[i].size() > 1)
              s_id = getRandomElement(students[i]);
            *it = "1 " + s_id;
            students[i].erase(s_id);
            erasePairsWithSuffix(enroll_pairs[i], s_id);
          }
          else
          {
            *it = "1 9999999"; // no-op
          }
        }
        // 2 <old>-<new> (update student ID cascade)
        else if (op == 2)
        {
          if (!students[i].empty())
          {
            std::string old_id = getRandomElement(students[i]);
            std::string new_id = pickFreshId(students[i], 7000000);
            *it = "2 " + old_id + "-" + new_id;
            students[i].erase(old_id);
            students[i].insert(new_id);
            replacePairsSuffix(enroll_pairs[i], old_id, new_id);
          }
        }
        // 3 <sid> <first> <last> (update student other)
        else if (op == 3)
        {
          std::string sid, first, last;
          iss >> sid >> first >> last;
          if (!students[i].empty())
          {
            sid = getRandomElement(students[i]);
            *it = "3 " + sid + " " + first + " " + last;
          }
        }
        // 4 <iid> <first> <last> <dept> (add instructor) -> rewrite dept to existing
        else if (op == 4)
        {
          std::string iid, first, last, dept;
          iss >> iid >> first >> last >> dept;

          if (!departments[i].empty())
          {
            std::string d = getRandomElement(departments[i]);
            *it = "4 " + iid + " " + first + " " + last + " " + d;
            free_departments[i].erase(d);
          }

          if (!iid.empty())
            instructors[i].insert(iid);
        }
        // 5 <iid> (delete instructor cascade) -> keep at least 1 instructor for Teach
        else if (op == 5)
        {
          if (instructors[i].size() > 1)
          {
            std::string iid = getRandomElement(instructors[i]);
            if (iid == seed_instructor[i] && instructors[i].size() > 1)
              iid = getRandomElement(instructors[i]);
            *it = "5 " + iid;
            instructors[i].erase(iid);
            erasePairsWithSuffix(teach_pairs[i], iid);
          }
          else
          {
            *it = "5 9999999"; // no-op
          }
        }
        // 6 <old>-<new> (update instructor ID cascade)
        else if (op == 6)
        {
          if (!instructors[i].empty())
          {
            std::string old_id = getRandomElement(instructors[i]);
            std::string new_id = pickFreshId(instructors[i], 8000000);
            *it = "6 " + old_id + "-" + new_id;
            instructors[i].erase(old_id);
            instructors[i].insert(new_id);
            replacePairsSuffix(teach_pairs[i], old_id, new_id);
          }
        }
        // 7 <iid> <first> <last> <dept> (update instructor other) -> rewrite iid + dept
        else if (op == 7)
        {
          std::string iid, first, last, dept;
          iss >> iid >> first >> last >> dept;

          if (!instructors[i].empty())
            iid = getRandomElement(instructors[i]);

          if (!departments[i].empty())
          {
            std::string d = getRandomElement(departments[i]);
            dept = d;
            free_departments[i].erase(d);
          }

          *it = "7 " + iid + " " + first + " " + last + " " + dept;
        }
        // 8 <cid> <cname> <sem> <dept> (add course force) -> rewrite dept; enforce cname unique by cname=cid
        else if (op == 8)
        {
          std::string cid, cname, sem, dept;
          iss >> cid >> cname >> sem >> dept;

          if (!departments[i].empty())
          {
            std::string d = getRandomElement(departments[i]);
            cname = cid; // keep unique course_name
            *it = "8 " + cid + " " + cname + " " + sem + " " + d;
            free_departments[i].erase(d);
          }

          if (!cid.empty())
          {
            courses[i].insert(cid);

            // update course_name sets
            course_names[i].insert(cname);
            course_id_to_name[i][cid] = cname;
          }
        }
        // 9 <cid> (delete course cascade) -> keep at least 1 course for Enroll/Teach
        else if (op == 9)
        {
          if (courses[i].size() > 1)
          {
            std::string cid = getRandomElement(courses[i]);
            if (cid == seed_course[i] && courses[i].size() > 1)
              cid = getRandomElement(courses[i]);
            *it = "9 " + cid;

            courses[i].erase(cid);
            erasePairsWithPrefix(enroll_pairs[i], cid);
            erasePairsWithPrefix(teach_pairs[i], cid);

            auto itn = course_id_to_name[i].find(cid);
            if (itn != course_id_to_name[i].end())
            {
              course_names[i].erase(itn->second);
              course_id_to_name[i].erase(itn);
            }
          }
          else
          {
            *it = "9 9999999"; // no-op
          }
        }
        // 10 <old>-<new> (update course ID cascade)
        else if (op == 10)
        {
          if (!courses[i].empty())
          {
            std::string old_id = getRandomElement(courses[i]);
            std::string new_id = pickFreshId(courses[i], 9000000);

            *it = "10 " + old_id + "-" + new_id;

            courses[i].erase(old_id);
            courses[i].insert(new_id);

            replacePairsPrefix(enroll_pairs[i], old_id, new_id);
            replacePairsPrefix(teach_pairs[i], old_id, new_id);

            auto itn = course_id_to_name[i].find(old_id);
            if (itn != course_id_to_name[i].end())
            {
              course_id_to_name[i].erase(itn);
              course_id_to_name[i][new_id] = itn->second;
            }
          }
        }
        // 11 <cid> <newname> (update course name force) -> rewrite cid + choose fresh newname
        else if (op == 11)
        {
          std::string cid, newname;
          iss >> cid >> newname;

          if (!courses[i].empty())
            cid = getRandomElement(courses[i]);

          // pick newname not in course_names
          do
          {
            newname = std::to_string(6000000 + rand_1000(gen));
          } while (course_names[i].find(newname) != course_names[i].end());

          *it = "11 " + cid + " " + newname;

          // Update local tracking
          auto itn = course_id_to_name[i].find(cid);
          if (itn != course_id_to_name[i].end())
          {
            course_names[i].erase(itn->second);
            itn->second = newname;
          }
          course_names[i].insert(newname);
          course_id_to_name[i][cid] = newname;
        }
        // 12 <cid> <sem> <dept> (update course other) -> must have existing course + dept
        else if (op == 12)
        {
          std::string cid, sem, dept;
          iss >> cid >> sem >> dept;

          if (!courses[i].empty())
            cid = getRandomElement(courses[i]);

          if (!departments[i].empty())
          {
            dept = getRandomElement(departments[i]);
            free_departments[i].erase(dept);
          }

          *it = "12 " + cid + " " + sem + " " + dept;
        }
        // 14 <dept> (delete dept no action) -> choose free dept if exists else choose non-existent (permissible)
        else if (op == 14)
        {
          if (!free_departments[i].empty())
          {
            std::string d = getRandomElement(free_departments[i]);
            *it = "14 " + d;
            free_departments[i].erase(d);
            departments[i].erase(d);
          }
          else
          {
            *it = "14 9999999"; // non-existent dept -> permissible no-op
          }
        }
        // 15 <cid>-<sid> (enroll) -> must have existing course + student
        else if (op == 15)
        {
          if (!courses[i].empty() && !students[i].empty())
          {
            std::string cid = getRandomElement(courses[i]);
            std::string sid = getRandomElement(students[i]);
            std::string p = cid + "-" + sid;
            *it = "15 " + p;
            enroll_pairs[i].insert(p);
          }
        }
        // 16 <cid>-<sid> (withdraw enrollment) -> use existing pair if possible
        else if (op == 16)
        {
          if (!enroll_pairs[i].empty())
          {
            std::string p = getRandomElement(enroll_pairs[i]);
            *it = "16 " + p;
            enroll_pairs[i].erase(p);
          }
          else
          {
            *it = "16 0-0";
          }
        }
        // 17 <cid>-<iid> (teach) -> must have existing course + instructor
        else if (op == 17)
        {
          if (!courses[i].empty() && !instructors[i].empty())
          {
            std::string cid = getRandomElement(courses[i]);
            std::string iid = getRandomElement(instructors[i]);
            std::string p = cid + "-" + iid;
            *it = "17 " + p;
            teach_pairs[i].insert(p);
          }
        }
        // 18 <cid>-<iid> (withdraw teaching) -> use existing pair if possible
        else if (op == 18)
        {
          if (!teach_pairs[i].empty())
          {
            std::string p = getRandomElement(teach_pairs[i]);
            *it = "18 " + p;
            teach_pairs[i].erase(p);
          }
          else
          {
            *it = "18 0-0";
          }
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
