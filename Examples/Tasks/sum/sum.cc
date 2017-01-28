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
  {
    TaskVariantRegistrar registrar(SUM_ID, "sum");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<sum_task>(registrar);
  }
  return Runtime::start(argc, argv);
}
