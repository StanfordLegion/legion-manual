#include <cstdio>
#include "legion.h"

using namespace Legion;
using namespace LegionRuntime::Arrays;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
};

enum FieldIDs {
  FIELD_A,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  // create a structured index space
  Rect<1> rec(Point<1>(0),Point<1>(99));
  IndexSpace sis = runtime->create_index_space(ctx,Domain::from_rect<1>(rec));

  // create a field space
  FieldSpace fs = runtime->create_field_space(ctx);
  FieldAllocator field_allocator = runtime->create_field_allocator(ctx,fs);
  FieldID fida = field_allocator.allocate_field(sizeof(float), FIELD_A);
  assert(fida == FIELD_A);

  // create two distinct logical regions
  LogicalRegion lr1 = runtime->create_logical_region(ctx,sis,fs);
  LogicalRegion lr2 = runtime->create_logical_region(ctx,sis,fs);

  // Clean up.  IndexAllocators and FieldAllocators automatically have their resources reclaimed
  // when they go out of scope.
  runtime->destroy_logical_region(ctx,lr1);
  runtime->destroy_logical_region(ctx,lr2);
  runtime->destroy_field_space(ctx,fs);
  runtime->destroy_index_space(ctx,sis);

}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  return Runtime::start(argc, argv);
}
