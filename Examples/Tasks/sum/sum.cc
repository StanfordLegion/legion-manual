#include <cstdio>
#include "legion.h"

using namespace Legion;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  SUM_ID,
};

void sum_task(const Task *task,
              const std::vector<PhysicalRegion> &regions,
              Context ctx, Runtime *runtime)
{
  int sum = 0;
  for (int i = 0; i <= 1000; i++) {
    sum += i;
  }
  printf("The sum of 0..1000 is %d\n", sum);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(SUM_ID);
  Runtime::register_legion_task<sum_task>(SUM_ID,
                                          Processor::LOC_PROC, 
                                          true/*single launch*/, 
                                          false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
