#include <cstdio>
#include "legion.h"

using namespace Legion;

enum TaskIDs {
  TOP_LEVEL_TASK_ID
};

enum FieldIDs {
  FIELD_A
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
  InlineLauncher launcher(RegionRequirement(lr, WRITE_DISCARD, EXCLUSIVE, lr).add_field(0,FIELD_A));
  PhysicalRegion pr = rt->map_region(ctx, launcher);
  pr.wait_until_valid();

  const FieldAccessor<WRITE_DISCARD,int,1> fa_a(pr, FIELD_A);
  for (PointInRectIterator<1> itr(rec); itr(); itr++)
    {
      fa_a[*itr] = 1;
    }
  int sum = 0;
  for (PointInRectIterator<1> itr(rec); itr(); itr++)
    {
      sum += fa_a[*itr];
    }
  printf("The sum of the elements of the region is %d\n",sum);

  // cleanup
  rt->unmap_region(ctx, pr);
  rt->destroy_logical_region(ctx,lr);
  rt->destroy_field_space(ctx,fs);
  rt->destroy_index_space(ctx,is);
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

