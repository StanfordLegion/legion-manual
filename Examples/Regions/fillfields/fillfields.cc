#include <cstdio>
#include "legion.h"

using namespace Legion;
using namespace LegionRuntime::Accessor;
using namespace LegionRuntime::Arrays;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  SUM_TASK_ID,
};

enum FieldIDs {
  FIELD_A,
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
  assert(fida == FIELD_A);


  LogicalRegion lr = runtime->create_logical_region(ctx,sis,fs);

  int init = 1;
  runtime->fill_field(ctx,lr,lr,fida,&init,sizeof(init));

  TaskLauncher sum_launcher(SUM_TASK_ID, TaskArgument(NULL,0));
  sum_launcher.add_region_requirement(RegionRequirement(lr, READ_ONLY, EXCLUSIVE, lr));
  sum_launcher.add_field(0, FIELD_A);
  runtime->execute_task(ctx, sum_launcher);

  // Clean up.  IndexAllocators and FieldAllocators automatically have their resources reclaimed
  // when they go out of scope.
  runtime->destroy_logical_region(ctx,lr);
  runtime->destroy_field_space(ctx,fs);
  runtime->destroy_index_space(ctx,sis);
}

void init_task(const Task *task,
                     const std::vector<PhysicalRegion> &regions,
                     Context ctx, Runtime *runtime)
{
  FieldID fid = FIELD_A;
  RegionAccessor<AccessorType::Generic, int> acc =
    regions[0].get_field_accessor(fid).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      acc.write(DomainPoint::from_point<1>(pir.p), 1);
    }
}

void sum_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, Runtime *runtime)
{
  FieldID fid = FIELD_A;
  RegionAccessor<AccessorType::Generic, int> acc =
    regions[0].get_field_accessor(fid).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  int sum = 0;
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      sum += acc.read(DomainPoint::from_point<1>(pir.p));
    }
  printf("The sum of the elements of the region is %d\n",sum);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  Runtime::register_legion_task<sum_task>(SUM_TASK_ID,
                                          Processor::LOC_PROC, 
                                          true/*single launch*/, 
                                          false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
