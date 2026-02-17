// File: ecros-courseware.cpp
#pragma once
#include "replicated-object.hpp"

// ---------------------------------------------------------------------------
// Stack-specific Ordana analysis stubs (ECRO Section 4.2)
// ---------------------------------------------------------------------------

inline bool
seqCommutative(const Call *a, const Call *b)
{
    if ((a->type == "AddStudentForce" && b->type == "AddStudentForce") ||
        (a->type == "AddStudentForce" && b->type == "DeleteStudentCascade") ||
        (a->type == "AddStudentForce" && b->type == "UpdateStudentIDCascade") ||
        (a->type == "AddStudentForce" && b->type == "UpdateStudentOther"))
        return false;
    if (a->type == "DeleteStudentCascade" && b->type == "AddStudentForce")
        return false;
    if ((a->type == "UpdateStudentIDCascade" && b->type == "AddStudentForce") ||
        (a->type == "UpdateStudentIDCascade" && b->type == "UpdateStudentIDCascade"))
        return false;
    if ((a->type == "UpdateStudentOther" && b->type == "AddStudentForce") ||
        (a->type == "UpdateStudentOther" && b->type == "UpdateStudentOther"))
        return false;

    if ((a->type == "AddInstructorForce" && b->type == "AddInstructorForce") ||
        (a->type == "AddInstructorForce" && b->type == "DeleteInstructorCascade") ||
        (a->type == "AddInstructorForce" && b->type == "UpdateInstructorIDCascade") ||
        (a->type == "AddInstructorForce" && b->type == "UpdateInstructorOther"))
        return false;
    if (a->type == "DeleteInstructorCascade" && b->type == "AddInstructorForce")
        return false;
    if ((a->type == "UpdateInstructorIDCascade" && b->type == "AddInstructorForce") ||
        (a->type == "UpdateInstructorIDCascade" && b->type == "UpdateInstructorIDCascade"))
        return false;
    if ((a->type == "UpdateInstructorOther" && b->type == "AddInstructorForce") ||
        (a->type == "UpdateInstructorOther" && b->type == "UpdateInstructorOther"))
        return false;

    if ((a->type == "AddCourseForce" && b->type == "AddInstructorForce") ||
        (a->type == "AddCourseForce" && b->type == "DeleteCourseCascade") ||
        (a->type == "AddCourseForce" && b->type == "UpdateCourseIDCascade") ||
        (a->type == "AddCourseForce" && b->type == "UpdateCourseNameForce") ||
        (a->type == "AddCourseForce" && b->type == "UpdateCourseOther"))
        return false;
    if (a->type == "DeleteCourseCascade" && b->type == "AddCourseForce")
        return false;
    if ((a->type == "UpdateCourseIDCascade" && b->type == "AddCourseForce") ||
        (a->type == "UpdateCourseIDCascade" && b->type == "UpdateCourseIDCascade"))
        return false;
    if ((a->type == "UpdateCourseNameForce" && b->type == "AddCourseForce") ||
        (a->type == "UpdateCourseNameForce" && b->type == "UpdateCourseNameForce"))
        return false;
    if ((a->type == "UpdateCourseOther" && b->type == "AddCourseForce") ||
        (a->type == "UpdateCourseOther" && b->type == "UpdateCourseOther"))
        return false;

    if (a->type == "Enroll" && b->type == "WithdrawEnrollment")
        return false;
    if (a->type == "WithdrawEnrollment" && b->type == "Enroll")
        return false;

    if (a->type == "Teach" && b->type == "WithdrawTeaching")
        return false;
    if (a->type == "WithdrawTeaching" && b->type == "Teach")
        return false;

    if (a->type == "AddDepartment" && b->type == "DeleteDepartmentNoAction")
        return false;
    if (a->type == "DeleteDepartmentNoAction" && b->type == "AddDepartment")
        return false;

    return true; // Default: commutative
}

inline bool commutative(const Call *a, const Call *b)
{
    return seqCommutative(a, b);
}

inline char resolution(const Call *a, const Call *b)
{

    // edge from enroll to delete
    // edge from b to a
    // if ((a->type == "DeleteCourse" || a->type == "DeleteStudent") && b->type == "Enroll")
    //     return '<';
    // edge frmo a to b
    // if (a->type == "Enroll" && (b->type == "DeleteCourse" || b->type == "DeleteStudent"))
    //    return '>';

    // edge from b to a
    if (a->type == "DeleteStudentCascade" && b->type == "Enroll")
        return '<';
    // edge from b to a
    if (a->type == "UpdateStudentIDCascade" && (b->type == "Enroll" || b->type == "UpdateStudentOther"))
        return '<';
    // edge from a to b
    if (a->type == "UpdateStudentOther" && b->type == "UpdateStudentIDCascade")
        return '>';

    // edge from a to b
    if (a->type == "AddInstructorForce" && b->type == "DeleteDepartmentNoAction")
        return '>';
    // edge from b to a
    if (a->type == "DeleteInstructorCascade" && b->type == "Teach")
        return '<';
    // edge from b to a
    if (a->type == "UpdateInstructorIDCascade" && (b->type == "Teach" || b->type == "UpdateInstructorOther"))
        return '<';
    // edge from a to b
    if (a->type == "UpdateInstructorOther" && (b->type == "UpdateInstructorIDCascade" || b->type == "DeleteDepartmentNoAction"))
        return '>';

    // edge from a to b
    if (a->type == "AddCourseForce" && b->type == "DeleteDepartmentNoAction")
        return '>';
    // edge from b to a
    if (a->type == "DeleteCourseCascade" && (b->type == "Enroll" || b->type == "Teach"))
        return '<';
    // edge from b to a
    if (a->type == "UpdateCourseIDCascade" && (b->type == "UpdateCourseNameForce" ||
                                               b->type == "UpdateCourseOther" ||
                                               b->type == "Enroll" ||
                                               b->type == "Teach"))
        return '<';
    // edge from a to b
    if (a->type == "UpdateCourseNameForce" && b->type == "UpdateCourseIDCascade")
        return '>';
    // edge from a to b
    if (a->type == "UpdateCourseOther" && (b->type == "UpdateCourseIDCascade" || b->type == "DeleteDepartmentNoAction"))
        return '>';

    // edge from a to b
    if (a->type == "Enroll" && (b->type == "DeleteStudentCascade" ||
                                b->type == "UpdateStudentIDCascade" ||
                                b->type == "DeleteCourseCascade" ||
                                b->type == "UpdateCourseIDCascade"))
        return '>';
    // edge from a to b
    if (a->type == "Teach" && (b->type == "DeleteInstructorCascade" ||
                               b->type == "UpdateInstructorIDCascade" ||
                               b->type == "DeleteCourseCascade" ||
                               b->type == "UpdateCourseIDCascade"))
        return '>';

    // edge from b to a
    if (a->type == "DeleteDepartmentNoAction" && (b->type == "AddInstructorForce" ||
                                                  b->type == "UpdateInstructorOther" ||
                                                  b->type == "AddCourseForce" ||
                                                  b->type == "UpdateCourseOther"))
        return '<';

    return '⊤';
}

inline char idcmp(const Call *a, const Call *b)
{
    // Deterministic tie-break: node_id, then call_id
    if (a->node_id < b->node_id)
        return '<';
    if (a->node_id > b->node_id)
        return '>';
    if (a->call_id < b->call_id)
        return '<';
    if (a->call_id > b->call_id)
        return '>';
    return '⊤';
}

// ---------------------------------------------------------------------------
//  Complexcourseware: holds state σ and implements apply/recompute for courseware use-case
// ---------------------------------------------------------------------------
class Complexcourseware
{
public:
    std::unordered_set<int> student_ids;
    std::unordered_set<int> course_ids;
    // std::unordered_set<std::pair<int, int>, pair_hash> enroll;
    bool send_ack;
    std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
    std::atomic<int> waittobestable;
    std::atomic<int> ackindex;
    Call send_ack_call;
    std::mutex pendMtx;
    std::map<int, Call> pendingLocalCalls; // call_id → Call
    std::set<std::pair<int, int>> applied;

    // --- Courseware tables (minimal, like your other protocol’s Complexcourseware) ---
    struct StudentRow
    {
        int first_name;
        int last_name;
    };
    struct InstructorRow
    {
        int first_name;
        int last_name;
        int department;
    };
    struct CourseRow
    {
        int course_name;
        int semester;
        int department;
    };

    std::unordered_map<int, StudentRow> students;
    std::unordered_map<int, InstructorRow> instructors;
    std::unordered_map<int, CourseRow> courses;
    std::unordered_set<int> departments;

    std::unordered_set<std::pair<int, int>, pair_hash> enroll; // <course_id, student_id>
    std::unordered_set<std::pair<int, int>, pair_hash> teach;  // <course_id, instructor_id>

    // UNIQUE course_name constraint: course_name -> course_id
    std::unordered_map<int, int> course_name_index;

    Complexcourseware()
    {
        // Initialize the courseware state
        student_ids.clear();
        course_ids.clear();
        send_ack = false;

        // Allocate memory for the array on the heap
        waittobestable.store(0);
        ackindex.store(0);
    };

    // Called by Handler::setVars to propagate node/node_count
    void setVars(int id, int nodes)
    {
        // Prefill a consistent base state so ops are rarely no-ops.
        students.clear();
        instructors.clear();
        courses.clear();
        departments.clear();
        enroll.clear();
        teach.clear();
        course_name_index.clear();
        applied.clear();

        for (int i = 0; i < 100; ++i)
        {
            departments.insert(i);
            students[i] = StudentRow{0, 0};
            instructors[i] = InstructorRow{0, 0, i}; // dept = i
            courses[i] = CourseRow{i, 0, i};         // course_name=i, semester=0, dept=i
            course_name_index[i] = i;                // unique name -> same id
        }
    }

    // Apply a single Call to this replica's courseware state
    void applyCall(Call *c)
    {
        std::pair<int, int> unique_id = {c->call_id, c->node_id};
        if (applied.count(unique_id))
            return;

        auto eraseEnrollByStudent = [&](int sid)
        {
            for (auto it = enroll.begin(); it != enroll.end();)
            {
                if (it->second == sid)
                    it = enroll.erase(it);
                else
                    ++it;
            }
        };

        auto eraseTeachByInstructor = [&](int iid)
        {
            for (auto it = teach.begin(); it != teach.end();)
            {
                if (it->second == iid)
                    it = teach.erase(it);
                else
                    ++it;
            }
        };

        auto eraseEnrollByCourse = [&](int cid)
        {
            for (auto it = enroll.begin(); it != enroll.end();)
            {
                if (it->first == cid)
                    it = enroll.erase(it);
                else
                    ++it;
            }
        };

        auto eraseTeachByCourse = [&](int cid)
        {
            for (auto it = teach.begin(); it != teach.end();)
            {
                if (it->first == cid)
                    it = teach.erase(it);
                else
                    ++it;
            }
        };

        auto deleteCourseCascadeLocal = [&](int cid)
        {
            auto itC = courses.find(cid);
            if (itC != courses.end())
            {
                int cname = itC->second.course_name;
                auto itIdx = course_name_index.find(cname);
                if (itIdx != course_name_index.end() && itIdx->second == cid)
                    course_name_index.erase(itIdx);
                courses.erase(itC);
            }
            eraseEnrollByCourse(cid);
            eraseTeachByCourse(cid);
        };

        // ------------------- Writes -------------------

        if (c->type == "AddDepartment")
        {
            departments.insert(c->value1);
        }
        else if (c->type == "DeleteDepartmentNoAction")
        {
            int dept = c->value1;

            // If dept doesn't exist -> no-op
            if (departments.find(dept) == departments.end())
            {
                // no-op
            }
            else
            {
                // Reject delete if referenced by any instructor
                bool referenced = false;
                for (const auto &kv : instructors)
                {
                    if (kv.second.department == dept)
                    {
                        referenced = true;
                        break;
                    }
                }
                // Reject delete if referenced by any course
                if (!referenced)
                {
                    for (const auto &kv : courses)
                    {
                        if (kv.second.department == dept)
                        {
                            referenced = true;
                            break;
                        }
                    }
                }

                if (!referenced)
                    departments.erase(dept); // allowed
                // else no-op
            }
        }

        else if (c->type == "AddStudentForce")
        {
            // Force semantics: overwrite same student_id (map overwrite is equivalent)
            StudentRow row{c->value2, c->value3};
            students[c->value1] = row;
        }
        else if (c->type == "DeleteStudentCascade")
        {
            int sid = c->value1;
            students.erase(sid);
            eraseEnrollByStudent(sid);
        }
        else if (c->type == "UpdateStudentIDCascade")
        {
            int old_id = c->value1;
            int new_id = c->value2;

            auto itS = students.find(old_id);
            if (itS != students.end())
            {
                StudentRow row = itS->second;
                students.erase(itS);
                students[new_id] = row;
            }

            // update ENROLLMENT (course_id, student_id)
            std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
            for (const auto &p : enroll)
            {
                if (p.second == old_id)
                    new_enroll.insert({p.first, new_id});
                else
                    new_enroll.insert(p);
            }
            enroll.swap(new_enroll);
        }
        else if (c->type == "UpdateStudentOther")
        {
            int sid = c->value1;
            auto itS = students.find(sid);
            if (itS != students.end())
            {
                itS->second.first_name = c->value2;
                itS->second.last_name = c->value3;
            }
            // else no-op
        }

        else if (c->type == "AddInstructorForce")
        {
            int dept = c->value4;
            if (departments.find(dept) != departments.end())
            {
                // Force overwrite on same instructor_id
                InstructorRow row{c->value2, c->value3, dept};
                instructors[c->value1] = row;
            }
            // else no-op (matches your permissibility idea)
        }
        else if (c->type == "DeleteInstructorCascade")
        {
            int iid = c->value1;
            instructors.erase(iid);
            eraseTeachByInstructor(iid);
        }
        else if (c->type == "UpdateInstructorIDCascade")
        {
            int old_id = c->value1;
            int new_id = c->value2;

            auto itI = instructors.find(old_id);
            if (itI != instructors.end())
            {
                InstructorRow row = itI->second;
                instructors.erase(itI);
                instructors[new_id] = row;
            }

            // update TEACHING (course_id, instructor_id)
            std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
            for (const auto &p : teach)
            {
                if (p.second == old_id)
                    new_teach.insert({p.first, new_id});
                else
                    new_teach.insert(p);
            }
            teach.swap(new_teach);
        }
        else if (c->type == "UpdateInstructorOther")
        {
            int iid = c->value1;
            auto itI = instructors.find(iid);
            if (itI != instructors.end())
            {
                itI->second.first_name = c->value2;
                itI->second.last_name = c->value3;
                itI->second.department = c->value4; // keep concept: allow update, even if dept missing
            }
            // else no-op
        }

        else if (c->type == "AddCourseForce")
        {
            int cid = c->value1;
            int cname = c->value2;
            int sem = c->value3;
            int dept = c->value4;

            if (departments.find(dept) != departments.end())
            {
                // If another course has same course_name, delete that course tuple (cascade links)
                auto itIdx = course_name_index.find(cname);
                if (itIdx != course_name_index.end() && itIdx->second != cid)
                {
                    int old_cid = itIdx->second;
                    deleteCourseCascadeLocal(old_cid);
                }

                // If this cid existed, remove its old name index entry (if consistent)
                auto itC = courses.find(cid);
                if (itC != courses.end())
                {
                    int old_name = itC->second.course_name;
                    auto itOldIdx = course_name_index.find(old_name);
                    if (itOldIdx != course_name_index.end() && itOldIdx->second == cid)
                        course_name_index.erase(itOldIdx);
                }

                // Overwrite index + row
                course_name_index[cname] = cid;
                courses[cid] = CourseRow{cname, sem, dept};
            }
            // else no-op
        }
        else if (c->type == "DeleteCourseCascade")
        {
            deleteCourseCascadeLocal(c->value1);
        }
        else if (c->type == "UpdateCourseIDCascade")
        {
            int old_id = c->value1;
            int new_id = c->value2;

            auto itC = courses.find(old_id);
            if (itC != courses.end())
            {
                CourseRow row = itC->second;
                courses.erase(itC);
                courses[new_id] = row;
                course_name_index[row.course_name] = new_id;
            }

            // update ENROLLMENT/TEACHING prefix (course_id, *)
            std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
            for (const auto &p : enroll)
            {
                if (p.first == old_id)
                    new_enroll.insert({new_id, p.second});
                else
                    new_enroll.insert(p);
            }
            enroll.swap(new_enroll);

            std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
            for (const auto &p : teach)
            {
                if (p.first == old_id)
                    new_teach.insert({new_id, p.second});
                else
                    new_teach.insert(p);
            }
            teach.swap(new_teach);
        }
        else if (c->type == "UpdateCourseNameForce")
        {
            int cid = c->value1;
            int new_name = c->value2;

            // If another course already has new_name, delete that tuple (cascade)
            auto itIdx = course_name_index.find(new_name);
            if (itIdx != course_name_index.end() && itIdx->second != cid)
            {
                int old_cid = itIdx->second;
                deleteCourseCascadeLocal(old_cid);
            }

            auto itC = courses.find(cid);
            if (itC != courses.end())
            {
                int old_name = itC->second.course_name;

                auto itOldIdx = course_name_index.find(old_name);
                if (itOldIdx != course_name_index.end() && itOldIdx->second == cid)
                    course_name_index.erase(itOldIdx);

                itC->second.course_name = new_name;
                course_name_index[new_name] = cid;
            }
            else
            {
                // keep index consistent like your other protocol’s current_state behavior
                course_name_index[new_name] = cid;
            }
        }
        else if (c->type == "UpdateCourseOther")
        {
            int cid = c->value1;
            int sem = c->value2;
            int dept = c->value3;

            auto itC = courses.find(cid);
            if (itC != courses.end() && departments.find(dept) != departments.end())
            {
                itC->second.semester = sem;
                itC->second.department = dept;
            }
            // else no-op
        }

        else if (c->type == "Enroll")
        {
            int cid = c->value1;
            int sid = c->value2;
            if (courses.find(cid) != courses.end() && students.find(sid) != students.end())
                enroll.insert({cid, sid});
            // else no-op
        }
        else if (c->type == "WithdrawEnrollment")
        {
            enroll.erase({c->value1, c->value2}); // no-op if missing
        }
        else if (c->type == "Teach")
        {
            int cid = c->value1;
            int iid = c->value2;
            if (courses.find(cid) != courses.end() && instructors.find(iid) != instructors.end())
                teach.insert({cid, iid});
            // else no-op
        }
        else if (c->type == "WithdrawTeaching")
        {
            teach.erase({c->value1, c->value2}); // no-op if missing
        }
        else if (c->type == "Read")
        {
            // no-op
        }

        applied.insert(unique_id);
    }

    bool shouldSkip(Call &call)
     {
    //     // Skip only when the op is guaranteed to be a no-op under current local state.

    //     if (call.type == "DeleteStudentCascade")
    //         return (students.find(call.value1) == students.end());

    //     if (call.type == "UpdateStudentIDCascade")
    //         return (students.find(call.value1) == students.end());

    //     if (call.type == "UpdateStudentOther")
    //         return (students.find(call.value1) == students.end());

    //     if (call.type == "DeleteInstructorCascade")
    //         return (instructors.find(call.value1) == instructors.end());

    //     if (call.type == "UpdateInstructorIDCascade")
    //         return (instructors.find(call.value1) == instructors.end());

    //     if (call.type == "UpdateInstructorOther")
    //         return (instructors.find(call.value1) == instructors.end());

    //     if (call.type == "DeleteCourseCascade")
    //         return (courses.find(call.value1) == courses.end());

    //     if (call.type == "UpdateCourseIDCascade")
    //         return (courses.find(call.value1) == courses.end());

    //     if (call.type == "UpdateCourseNameForce")
    //         return (courses.find(call.value1) == courses.end() && course_name_index.find(call.value2) == course_name_index.end());

    //     if (call.type == "UpdateCourseOther")
    //         return (courses.find(call.value1) == courses.end() ||
    //                 departments.find(call.value3) == departments.end());

    //     if (call.type == "AddInstructorForce")
    //         return (departments.find(call.value4) == departments.end());

    //     if (call.type == "AddCourseForce")
    //         return (departments.find(call.value4) == departments.end());

    //     if (call.type == "DeleteDepartmentNoAction")
    //     {
    //         int dept = call.value1;

    //         // If dept doesn't exist -> guaranteed no-op
    //         if (departments.find(dept) == departments.end())
    //             return true;

    //         // If referenced -> no-op (cannot delete)
    //         for (const auto &kv : instructors)
    //             if (kv.second.department == dept)
    //                 return true;

    //         for (const auto &kv : courses)
    //             if (kv.second.department == dept)
    //                 return true;

    //         return false; // deletable
    //     }

    //     if (call.type == "Enroll")
    //         return !(courses.find(call.value1) != courses.end() &&
    //                  students.find(call.value2) != students.end());

    //     if (call.type == "Teach")
    //         return !(courses.find(call.value1) != courses.end() &&
    //                  instructors.find(call.value2) != instructors.end());

    //     if (call.type == "WithdrawEnrollment")
    //         return (enroll.find({call.value1, call.value2}) == enroll.end());

    //     if (call.type == "WithdrawTeaching")
    //         return (teach.find({call.value1, call.value2}) == teach.end());

    //     // AddStudentForce/AddDepartment/Read usually should NOT be skipped.

    //To test performance of system, make it comment comment.
        return false;
    }

    // Replay the entire topoOrder to rebuild state from σ₀
    void recomputeState(const std::vector<Call *> &order)
    {
        // rebuild σ from scratch
        students.clear();
        instructors.clear();
        courses.clear();
        departments.clear();
        enroll.clear();
        teach.clear();
        course_name_index.clear();

        applied.clear();

        for (Call *call : order)
        {
            applyCall(call);
        }
    }

    enum CoursewareEnum
    {
        AddStudentForce,
        DeleteStudentCascade,
        UpdateStudentIDCascade,
        UpdateStudentOther,

        AddInstructorForce,
        DeleteInstructorCascade,
        UpdateInstructorIDCascade,
        UpdateInstructorOther,

        AddCourseForce,
        DeleteCourseCascade,
        UpdateCourseIDCascade,
        UpdateCourseNameForce,
        UpdateCourseOther,

        AddDepartment,
        DeleteDepartmentNoAction,

        Enroll,
        WithdrawEnrollment,

        Teach,
        WithdrawTeaching,

        Read
    };

    std::string coursewareToString(int courseware)
    {
        switch (static_cast<CoursewareEnum>(courseware))
        {
        case AddStudentForce:
            return "AddStudentForce";
        case DeleteStudentCascade:
            return "DeleteStudentCascade";
        case UpdateStudentIDCascade:
            return "UpdateStudentIDCascade";
        case UpdateStudentOther:
            return "UpdateStudentOther";

        case AddInstructorForce:
            return "AddInstructorForce";
        case DeleteInstructorCascade:
            return "DeleteInstructorCascade";
        case UpdateInstructorIDCascade:
            return "UpdateInstructorIDCascade";
        case UpdateInstructorOther:
            return "UpdateInstructorOther";

        case AddCourseForce:
            return "AddCourseForce";
        case DeleteCourseCascade:
            return "DeleteCourseCascade";
        case UpdateCourseIDCascade:
            return "UpdateCourseIDCascade";
        case UpdateCourseNameForce:
            return "UpdateCourseNameForce";
        case UpdateCourseOther:
            return "UpdateCourseOther";

        case AddDepartment:
            return "AddDepartment";
        case DeleteDepartmentNoAction:
            return "DeleteDepartmentNoAction";

        case Enroll:
            return "Enroll";
        case WithdrawEnrollment:
            return "WithdrawEnrollment";

        case Teach:
            return "Teach";
        case WithdrawTeaching:
            return "WithdrawTeaching";

        case Read:
            return "Read";
        default:
            return "Invalid";
        }
    }
    void readBenchFile(const std::string &filename, int &expected_calls, int id, int numnodes, Call call, std::vector<Call> &calls)
    {
        std::ifstream infile(filename);
        // number_of_nodes = numnodes;
        if (!infile)
        {
            std::cerr << "Could not open file: " << filename << std::endl;
            return;
        }

        std::string line;
        if (std::getline(infile, line))
        {
            // Remove the '#' character and convert to integer
            expected_calls = std::stoi(line.substr(1));
            // std::cout << "readBenchFile--" << "expected_calls :" << expected_calls << std::endl;
        }
        int call_id_counter = 0;
        while (std::getline(infile, line))
        {
            // Call call;
            // size_t spacePos = line.find(' ');
            // call.type = projectToString(std::stoi(line.substr(0, spacePos)));
            //::cout << "readBenchFile--" << "expected_calls :" << call.type << std::endl;

            std::istringstream iss(line);
            int tmp_op;
            std::string temp;

            iss >> tmp_op;
            call.type = coursewareToString(tmp_op);
            // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;

            // defaults
            call.value1 = 0;
            call.value2 = 0;
            call.value3 = 0;
            call.value4 = 0;

            if (call.type != "Read")
            {
                call_id_counter++;
                call.call_id = call_id_counter;

                if (call.type == "Enroll" || call.type == "Teach" || call.type == "WithdrawEnrollment" || call.type == "WithdrawTeaching" ||
                    call.type == "UpdateStudentIDCascade" || call.type == "UpdateInstructorIDCascade" || call.type == "UpdateCourseIDCascade")
                {
                    std::getline(iss, temp, '-');
                    call.value1 = std::stoi(temp);
                    iss >> call.value2;
                }
                else if (call.type == "AddStudentForce" || call.type == "UpdateStudentOther")
                {
                    iss >> call.value1;
                    iss >> call.value2;
                    iss >> call.value3;
                }
                else if (call.type == "AddInstructorForce" || call.type == "UpdateInstructorOther")
                {
                    iss >> call.value1;
                    iss >> call.value2;
                    iss >> call.value3;
                    iss >> call.value4;
                }
                else if (call.type == "AddCourseForce")
                {
                    iss >> call.value1;
                    iss >> call.value2;
                    iss >> call.value3;
                    iss >> call.value4;
                }
                else if (call.type == "UpdateCourseOther")
                {
                    iss >> call.value1;
                    iss >> call.value2;
                    iss >> call.value3;
                }
                else if (call.type == "UpdateCourseNameForce")
                {
                    iss >> call.value1;
                    iss >> call.value2;
                }
                else
                {
                    // single-arg ops: DeleteStudentCascade, DeleteInstructorCascade, DeleteCourseCascade,
                    // AddDepartment, DeleteDepartmentNoAction, etc.
                    iss >> call.value1;
                }
            }

            call.node_id = id;
            // node_id = id;
            call.stable = false;
            calls.push_back(call);
        }
    }

    bool checkValidcall(Call call)
    {
        if (call.type == "AddStudentForce" ||
            call.type == "DeleteStudentCascade" ||
            call.type == "UpdateStudentIDCascade" ||
            call.type == "UpdateStudentOther" ||

            call.type == "AddInstructorForce" ||
            call.type == "DeleteInstructorCascade" ||
            call.type == "UpdateInstructorIDCascade" ||
            call.type == "UpdateInstructorOther" ||

            call.type == "AddCourseForce" ||
            call.type == "DeleteCourseCascade" ||
            call.type == "UpdateCourseIDCascade" ||
            call.type == "UpdateCourseNameForce" ||
            call.type == "UpdateCourseOther" ||

            call.type == "AddDepartment" ||
            call.type == "DeleteDepartmentNoAction" ||

            call.type == "Enroll" ||
            call.type == "WithdrawEnrollment" ||

            call.type == "Teach" ||
            call.type == "WithdrawTeaching" ||

            call.type == "Read")
            return true;
        else
            return false;
    }
};
