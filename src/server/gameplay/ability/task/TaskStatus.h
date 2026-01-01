//
// Created by tmsz on 26-1-1.
//

#ifndef DEMO0_SERVER_TASKSTATUS_H
#define DEMO0_SERVER_TASKSTATUS_H
#include <cstdint>

namespace ability
{
    enum class TaskStatus : uint8_t
    {
        Running,
        Succeeded,
        Failed,
        Canceled,
    };

    struct TaskTickResult
    {
        TaskStatus status = TaskStatus::Running;
    };
}
#endif //DEMO0_SERVER_TASKSTATUS_H
