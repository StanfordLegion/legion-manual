#include <cstdio>
#include "legion.h"

using namespace Legion;
//using namespace LegionRuntime::Arrays;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  PRODUCER_ID,
  CONSUMER_ID
};

//
// The recommended way to use index launches is to build on the base IndexLauncher class to define
// launcher objects that encapsulate all of the information specific to launching index tasks of 
// a particular kind.  Here we define a launcher class for the producer tasks.
//
class ProducerTasks: public IndexLauncher {
public:
  ProducerTasks(const Domain &launch_domain, const ArgumentMap &arg_map);
public:
  static const int TASK_ID = PRODUCER_ID;  // The id of the tasks that will be launched.
  static const int MAPPER_ID = 0;
};

ProducerTasks::ProducerTasks(const Domain &launch_domain,const ArgumentMap &arg_map) : 
  IndexLauncher(PRODUCER_ID,          // the ID of the tasks this index launcher will execute
		launch_domain,        // identities of the tasks to launch (an index space)
                TaskArgument(NULL,0), // arguments passed to all the tasks, not used in this example
                arg_map,              // a map from the launch domain to the argument for each individual task
                Predicate::TRUE_PRED, // these tasks can be indexed launched
                false,                // these tasks cannot be single launched
                0)                    // an id for the mapper --- not used in this example
{

}

class ConsumerTasks: public IndexLauncher {
public:
  ConsumerTasks(const Domain &launch_domain,
	       const ArgumentMap &arg_map);
public:
  static const int TASK_ID = CONSUMER_ID;
  static const int MAPPER_ID = 0;
};

ConsumerTasks::ConsumerTasks(const Domain &launch_domain,const ArgumentMap &arg_map) : 
   IndexLauncher(CONSUMER_ID, launch_domain, TaskArgument(NULL,0), arg_map, Predicate::TRUE_PRED, false, 0)
{

}

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)

{
  // Launch 50 tasks.
  int points = 50;
  const Rect<1> launch_domain(1,points);
  ArgumentMap producer_arg_map;
  for (int i = 0; i < points; i += 1)
  {
    int subtask_id = 2*i;
    producer_arg_map.set_point(DomainPoint::from_point<1>(i+1), TaskArgument(&subtask_id,sizeof(int)));
  }
  ProducerTasks  producer_launcher(launch_domain, producer_arg_map);
  // 
  //  Since each producer task returns an integer, the index launch will return a FutureMap, a map from
  //  the launch domain to a future for each point.  The FutureMap can be used as an ArgumentMap to a subsequent
  //  index launch.
  //
  FutureMap fm = runtime->execute_index_space(ctx, producer_launcher);
  ArgumentMap consumer_arg_map(fm);
  ConsumerTasks consumer_launcher(launch_domain, consumer_arg_map);
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
    TaskVariantRegistrar registrar(PRODUCER_ID, "index_producer");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<int,subtask_producer>(registrar);
  }
  {
    TaskVariantRegistrar registrar(CONSUMER_ID, "index_consumer");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<subtask_consumer>(registrar);
  }
  return Runtime::start(argc, argv);
}
