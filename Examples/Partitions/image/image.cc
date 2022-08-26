#include <cstdio>
#include "legion.h"

using namespace Legion;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  SUM_TASK_ID,
  PTR_TASK_ID,
};

enum FieldIDs {
  FIELD_VAL,
  FIELD_PTR,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &rgns,
		    Context ctx, 
		    Runtime *rt)
{
  Rect<1> rec(Point<1>(0),Point<1>(99));
  IndexSpace is = rt->create_index_space(ctx,rec);
  FieldSpace fs1 = rt->create_field_space(ctx);
  FieldAllocator field_allocator1 = rt->create_field_allocator(ctx,fs1);
  FieldID fidptr = field_allocator1.allocate_field(sizeof(Point<1>), FIELD_PTR);

  FieldSpace fs2 = rt->create_field_space(ctx);
  FieldAllocator field_allocator2 = rt->create_field_allocator(ctx,fs2);  
  FieldID fidv = field_allocator2.allocate_field(sizeof(int), FIELD_VAL);
  
  assert(fidptr == FIELD_PTR);
  assert(fidv == FIELD_VAL);

  LogicalRegion lr_src = rt->create_logical_region(ctx,is,fs1);
  LogicalRegion lr_dst = rt->create_logical_region(ctx,is,fs2);

  int init = 1;
  rt->fill_field(ctx,lr_dst,lr_dst,FIELD_VAL,&init,sizeof(init));

  int num_subregions = 4;
  Rect<1> colors(0,num_subregions-1);
  IndexSpace cis = rt->create_index_space(ctx,colors);

  IndexPartition ip_src = rt->create_equal_partition(ctx, is ,cis);
  LogicalPartition lp_src = rt->get_logical_partition(ctx, lr_src, ip_src);

  ArgumentMap arg_map;
  IndexLauncher ptr_launcher(PTR_TASK_ID, colors, TaskArgument(NULL,0), arg_map);
  ptr_launcher.add_region_requirement(RegionRequirement(lp_src, 0, WRITE_DISCARD, EXCLUSIVE, lr_src));
  ptr_launcher.region_requirements[0].add_field(FIELD_PTR);
  rt->execute_index_space(ctx, ptr_launcher);

  IndexPartition ip_dst = rt->create_partition_by_image(ctx, is, lp_src, lr_src, FIELD_PTR, cis);
  LogicalPartition lp_dst = rt->get_logical_partition(ctx, lr_dst, ip_dst);

  IndexLauncher sum_launcher(SUM_TASK_ID, colors, TaskArgument(NULL,0), arg_map);
  sum_launcher.add_region_requirement(RegionRequirement(lp_dst, 0, READ_ONLY, EXCLUSIVE, lr_dst));
  sum_launcher.region_requirements[0].add_field(FIELD_VAL);
  rt->execute_index_space(ctx, sum_launcher); 
}

void ptr_task(const Task *task,
               const std::vector<PhysicalRegion> &rgns,
               Context ctx, Runtime *rt)
{
  const FieldAccessor<WRITE_DISCARD,Point<1>,1> fa_ptr(rgns[0], FIELD_PTR);
  Rect<1> d = rt->get_index_space_domain(ctx, task->regions[0].region.get_index_space());

  int x = 0;
  for (PointInRectIterator<1> itr(d); itr(); itr++)
    {
      fa_ptr[*itr] =  (Point<1>) x;
      x = x + 1;
    }
}

void sum_task(const Task *task,
		    const std::vector<PhysicalRegion> &rgns,
		    Context ctx, Runtime *rt)
{
  const FieldAccessor<READ_ONLY,int,1> fa_a(rgns[0], FIELD_VAL);
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
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(SUM_TASK_ID, "sum_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<sum_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(PTR_TASK_ID, "ptr_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<ptr_task>(registrar);
  }
  return Runtime::start(argc, argv);
}

