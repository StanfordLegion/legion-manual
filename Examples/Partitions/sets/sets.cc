#include <cstdio>
#include "legion.h"

using namespace Legion;


enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  SUM_TASK_ID,
  COLOR_TASK_ID,
};

enum FieldIDs {
  FIELD_A,
  FIELD_PARTITION
};

struct ColorArg {
  Rect<1> colors;
  int offset;
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
  FieldID fidp = field_allocator.allocate_field(sizeof(Point<1>), FIELD_PARTITION);
  assert(fida == FIELD_A);
  assert(fidp == FIELD_PARTITION);
  
  LogicalRegion lr = rt->create_logical_region(ctx,is,fs);

  int init = 1;
  rt->fill_field(ctx,lr,lr,fida,&init,sizeof(init));

  int num_subregions = 4;
  Rect<1> colors(0,num_subregions-1);
  ColorArg ca = { colors, 10 };
  IndexSpace cis = rt->create_index_space(ctx,colors);

  IndexPartition eip = rt->create_equal_partition(ctx, is, cis);

  TaskLauncher color_launcher(COLOR_TASK_ID, TaskArgument(&ca,sizeof(ColorArg)));
  color_launcher.add_region_requirement(RegionRequirement(lr, WRITE_DISCARD, EXCLUSIVE, lr));
  color_launcher.add_field(0,FIELD_PARTITION);
  rt->execute_task(ctx, color_launcher);

  IndexPartition fip = rt->create_partition_by_field(ctx, lr, lr, FIELD_PARTITION,cis);


  IndexPartition iip = rt->create_partition_by_intersection(ctx,is,eip,fip,cis);
  LogicalPartition ilp = rt->get_logical_partition(ctx, lr, iip);
  ArgumentMap arg_map;
  IndexLauncher sum_launcher(SUM_TASK_ID, colors, TaskArgument(NULL,0), arg_map);
  sum_launcher.add_region_requirement(RegionRequirement(ilp, 0, READ_ONLY, EXCLUSIVE, lr));
  sum_launcher.region_requirements[0].add_field(FIELD_A);
  rt->execute_index_space(ctx, sum_launcher); 
}

void color_task(const Task *task,
               const std::vector<PhysicalRegion> &rgns,
               Context ctx, Runtime *rt)
{
  //  
  //  FieldAccessor is templated on privilege, field type, and number of index space dimensions
  //

  ColorArg ca = *((ColorArg *) task->args);
  int divisor = (ca.colors.hi - ca.colors.lo) + 1;
  const FieldAccessor<WRITE_DISCARD,Point<1>,1> fa_p(rgns[0], FIELD_PARTITION);
  Rect<1> d = rt->get_index_space_domain(ctx, task->regions[0].region.get_index_space());
  int quarter = ((int) (d.hi-d.lo) + 1) / divisor; // assume this number is an integer
  int x = ca.offset;
  
  for (PointInRectIterator<1> itr(d); itr(); itr++)
    {
      int c = (int) (x / quarter);
      fa_p[*itr] =  (c < (int) d.hi) ? (Point<1>) c : d.hi;
      x = x + 1;
    }
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
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(SUM_TASK_ID, "sum_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<sum_task>(registrar);
  }
  {
    TaskVariantRegistrar registrar(COLOR_TASK_ID, "color_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<color_task>(registrar);
  }
  return Runtime::start(argc, argv);
}




