#include <cstdio>
#include "legion.h"

using namespace Legion;
using namespace LegionRuntime::Accessor;
using namespace LegionRuntime::Arrays;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  INC_TASK_ID_BOTH,
};

enum FieldIDs {
  FIELD_A,
  FIELD_B,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  Rect<1> rec(Point<1>(0),Point<1>(99));
  IndexSpace sis = runtime->create_index_space(ctx,Domain::from_rect<1>(rec));
  FieldSpace fs = runtime->create_field_space(ctx);
  FieldAllocator field_allocator = runtime->create_field_allocator(ctx,fs);
  FieldID fida = field_allocator.allocate_field(sizeof(int), FIELD_A);
  FieldID fidb = field_allocator.allocate_field(sizeof(int), FIELD_B);
  assert(fida == FIELD_A);
  assert(fidb == FIELD_B);

  LogicalRegion lr = runtime->create_logical_region(ctx,sis,fs);

  int tasklaunch = 1;
  TaskLauncher launcher1(INC_TASK_ID_BOTH, TaskArgument(&tasklaunch, sizeof(tasklaunch)));
  RegionRequirement rr(lr, WRITE_DISCARD, EXCLUSIVE, lr);
  rr.add_field(FIELD_A);
  rr.add_field(FIELD_B);
  launcher1.add_region_requirement(rr);
  runtime->execute_task(ctx, launcher1);

  tasklaunch = 2;
  TaskLauncher launcher2(INC_TASK_ID_BOTH, TaskArgument(&tasklaunch, sizeof(tasklaunch)));
  RegionRequirement rrb(lr, WRITE_DISCARD, EXCLUSIVE, lr);
  rrb.add_field(FIELD_B);
  launcher2.add_region_requirement(rrb);
  RegionRequirement rra(lr, WRITE_DISCARD, EXCLUSIVE, lr);
  rra.add_field(FIELD_A);
  launcher2.add_region_requirement(rra);
  runtime->execute_task(ctx, launcher2);
  }

void inc_task_field_both(const Task *task,
			 const std::vector<PhysicalRegion> &regions,
			 Context ctx, Runtime *runtime)
{
  RegionAccessor<AccessorType::Generic, int> acca =
    regions[0].get_field_accessor(FIELD_A).typeify<int>();

  RegionAccessor<AccessorType::Generic, int> accb =
    regions[0].get_field_accessor(FIELD_B).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      accb.write(DomainPoint::from_point<1>(pir.p),1);
      acca.write(DomainPoint::from_point<1>(pir.p),1);
    }
  printf("Task %d done.\n", *((int *) task->args));
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<inc_task_field_both>(INC_TASK_ID_BOTH,
                                                     Processor::LOC_PROC, 
                                                     true/*single launch*/, 
                                                     false/*no multiple launch*/);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
