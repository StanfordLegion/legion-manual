#include <cstdio>
#include "legion.h"

using namespace Legion;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  SUBTASK_PRODUCER_ID,
  SUBTASK_CONSUMER_ID,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  printf("Top level task start.\n");
  for(int i = 1; i <= 100; i += 2) {
    TaskLauncher producer_launcher(SUBTASK_PRODUCER_ID, TaskArgument(&i,sizeof(int)));
    Future doubled_task_number =  runtime->execute_task(ctx,producer_launcher);
    TaskLauncher consumer_launcher(SUBTASK_CONSUMER_ID, TaskArgument(NULL,0));
    consumer_launcher.add_future(doubled_task_number);
    runtime->execute_task(ctx,consumer_launcher);
  }
  printf("Top level task done launching subtasks.\n");
}

int subtask_producer(const Task *task,
		      const std::vector<PhysicalRegion> &regions,
		      Context ctx, 
		      Runtime *runtime)
{
  int subtask_number = *((int *) task->args);
  printf("\tProducer subtask %d\n", subtask_number);
  return subtask_number + 1;
}

void subtask_consumer(const Task *task,
		     const std::vector<PhysicalRegion> &regions,
		     Context ctx, 
		     Runtime *runtime)
{
  Future f = task->futures[0];
  int subtask_number = f.get_result<int>();
  printf("\tConsumer subtask %d\n", subtask_number);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  Runtime::register_legion_task<int,subtask_producer>(SUBTASK_PRODUCER_ID,
						      Processor::LOC_PROC, 
						      true/*single launch*/, 
						      false/*no multiple launch*/);
  Runtime::register_legion_task<subtask_consumer>(SUBTASK_CONSUMER_ID,
						  Processor::LOC_PROC, 
						  true/*single launch*/, 
						  false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
