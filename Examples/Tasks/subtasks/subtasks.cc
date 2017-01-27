#include <cstdio>
#include "legion.h"

using namespace Legion;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  SUBTASK_ID
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  printf("Top level task start.\n");
  for(int i = 1; i <= 100; i++) {
    TaskLauncher launcher(SUBTASK_ID, TaskArgument(&i,sizeof(int)));
    runtime->execute_task(ctx,launcher);
  }
  printf("Top level task ddone launching subtasks.\n");
}

void subtask(const Task *task,
	     const std::vector<PhysicalRegion> &regions,
	     Context ctx, 
	     Runtime *runtime)
{
  int subtask_number = *((int *) task->args);
  printf("\tSubtask %d\n", subtask_number);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  Runtime::register_legion_task<subtask>(SUBTASK_ID,
                                         Processor::LOC_PROC, 
                                         true/*single launch*/, 
                                         false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
