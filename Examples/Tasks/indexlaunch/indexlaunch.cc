#include <cstdio>
#include "legion.h"

using namespace Legion;
//using namespace LegionRuntime::Arrays;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  INDEX_PRODUCER_ID,
  INDEX_CONSUMER_ID,
};

class PCTask: public IndexLauncher {
public:
  PCTask(const Domain &launch_domain,
	 const ArgumentMap &arg_map);
public:
void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  int points = 50;
  const Rect<1> launch_domain(1,points);
  ArgumentMap producer_arg_map;
  for (int i = 0; i < points; i += 1)
  {
    int subtask_id = 2*i;
    producer_arg_map.set_point(DomainPoint::from_point<1>(i+1),
			       TaskArgument(&subtask_id,sizeof(int)));
  }
  IndexLauncher producer_launcher(INDEX_PRODUCER_ID,
				  launch_domain,
				  TaskArgument(NULL, 0),
				  producer_arg_map);
  FutureMap fm = runtime->execute_index_space(ctx, producer_launcher);
  ArgumentMap consumer_arg_map(fm);
  IndexLauncher consumer_launcher(INDEX_CONSUMER_ID,
				  launch_domain,
				  TaskArgument(NULL, 0),
				  consumer_arg_map);
  runtime->execute_index_space(ctx, consumer_launcher);
}

int subtask_producer(const Task *task,
		     const std::vector<PhysicalRegion> &regions,
		     Context ctx,
		     Runtime *runtime)
{
  int subtask_number = *((const int *)task->local_args);
  printf("\tProducer subtask %d\n", subtask_number);
  return subtask_number + 1;
}

void subtask_consumer(const Task *task,
		      const std::vector<PhysicalRegion> &regions,
		      Context ctx,
		      Runtime *runtime)
{
  int subtask_number = *((const int*)task->local_args);
  printf("\tConsumer subtask %d\n", subtask_number);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(INDEX_PRODUCER_ID, "index_producer");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<int,subtask_producer>(registrar);
  }
  {
    TaskVariantRegistrar registrar(INDEX_CONSUMER_ID, "index_consumer");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<subtask_consumer>(registrar);
  }
  return Runtime::start(argc, argv);
}
