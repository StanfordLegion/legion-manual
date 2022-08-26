#include <cstdio>
#include "legion.h"

using namespace Legion;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  SUM_TASK_ID,
};

enum FieldIDs {
  FIELD_A,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &rgns,
		    Context ctx, 
		    Runtime *rt)
{
  Rect<1> rec(Point<1>(0),Point<1>(99));
  IndexSpace is = rt->create_index_space(ctx,rec);
  FieldSpace fs = rt->create_field_space(ctx);
  FieldAllocator field_allocator = rt->create_field_allocator(ctx,fs);
  FieldID fida = field_allocator.allocate_field(sizeof(int), FIELD_A);
  assert(fida == FIELD_A);


  LogicalRegion lr = rt->create_logical_region(ctx,is,fs);
  int num_subregions = 4;
  Rect<1> colors(0,num_subregions - 1);
  IndexSpace color_is = rt->create_index_space(ctx, colors);
  IndexPartition ip = rt->create_equal_partition(ctx, is, color_is);
  LogicalPartition lp = rt->get_logical_partition(ctx, lr, ip);
  
  int init = 1;
  rt->fill_field(ctx,lr,lr,fida,&init,sizeof(init));

  ArgumentMap arg_map;
  IndexLauncher sum_launcher(SUM_TASK_ID, colors, TaskArgument(NULL,0), arg_map);
  sum_launcher.add_region_requirement(RegionRequirement(lp, 0, READ_ONLY, EXCLUSIVE, lr));
  sum_launcher.region_requirements[0].add_field(FIELD_A);
  rt->execute_index_space(ctx, sum_launcher);

  // Clean up.  IndexAllocators and FieldAllocators automatically have their resources reclaimed
  // when they go out of scope.
  rt->destroy_logical_region(ctx,lr);
  rt->destroy_field_space(ctx,fs);
  rt->destroy_index_space(ctx,is);
}

void sum_task(const Task *task,
		    const std::vector<PhysicalRegion> &rgns,
		    Context ctx, Runtime *rt)
{
  const FieldAccessor<READ_ONLY,int,1> fa_a(rgns[0], FIELD_A);
  Rect<1> d = rt->get_index_space_domain(ctx,task->regions[0].region.get_index_space());
  int sum = 0;
  for (PointInRectIterator<1> itr(d); itr(); itr++)
    {
      sum += fa_a[*itr]; 
    }
  printf("The sum of the elements of the region is %d\n",sum);
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    registrar.set_replicable();   // The only change from Examples/Partitions/equal/equal.cc
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(SUM_TASK_ID, "sum_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<sum_task>(registrar);
  }
  return Runtime::start(argc, argv);
}

