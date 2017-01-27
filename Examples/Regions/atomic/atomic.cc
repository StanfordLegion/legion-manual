#include <cstdio>
#include "legion.h"

using namespace Legion;
using namespace LegionRuntime::Accessor;
using namespace LegionRuntime::Arrays;

enum TaskIDs {
  TOP_LEVEL_TASK_ID,
  INC_TASK_ID_FIELDA,
  INC_TASK_ID_FIELDB,
  INC_TASK_ID_BOTH,
  SUM_TASK_ID,
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
  Rect<1> rec(Point<1>(0),Point<1>(999));
  IndexSpace sis = runtime->create_index_space(ctx,Domain::from_rect<1>(rec));
  FieldSpace fs = runtime->create_field_space(ctx);
  FieldAllocator field_allocator = runtime->create_field_allocator(ctx,fs);
  FieldID fida = field_allocator.allocate_field(sizeof(int), FIELD_A);
  FieldID fidb = field_allocator.allocate_field(sizeof(int), FIELD_B);
  assert(fida == FIELD_A);
  assert(fidb == FIELD_B);

  LogicalRegion lr = runtime->create_logical_region(ctx,sis,fs);

  int init = 1;
  runtime->fill_field(ctx,lr,lr,fida,&init,sizeof(init));
  runtime->fill_field(ctx,lr,lr,fidb,&init,sizeof(init));

  TaskLauncher inc_launcher_fielda_only(INC_TASK_ID_FIELDA, TaskArgument(NULL,0));
  RegionRequirement rra(lr, READ_WRITE, ATOMIC, lr);
  rra.add_field(FIELD_A);
  inc_launcher_fielda_only.add_region_requirement(rra);

  TaskLauncher inc_launcher_fieldb_only(INC_TASK_ID_FIELDB, TaskArgument(NULL,0));
  RegionRequirement rrb(lr, READ_WRITE, EXCLUSIVE, lr);
  rrb.add_field(FIELD_B);
  inc_launcher_fieldb_only.add_region_requirement(rrb);

  TaskLauncher inc_launcher_field_both(INC_TASK_ID_BOTH, TaskArgument(NULL,0));
  RegionRequirement rrbotha(lr, READ_WRITE, ATOMIC, lr);
  rrbotha.add_field(FIELD_A);
  inc_launcher_field_both.add_region_requirement(rrbotha);
  RegionRequirement rrbothb(lr, READ_ONLY, ATOMIC, lr);
  rrbothb.add_field(FIELD_B);
  inc_launcher_field_both.add_region_requirement(rrbothb);


  //
  // Launch a bunch of tasks that will update FIELD_B
  //
  for(int i = 0; i < 100; i++) {
    inc_launcher_fieldb_only.argument = TaskArgument(&i,sizeof(i));
    runtime->execute_task(ctx, inc_launcher_fieldb_only);
  }

  //
  // Now launch pairs of tasks, one of which reads from FIELD_B (and so has dependencies
  // on the previous collection of tasks) and FIELD_A, and one of which only reads from FIELD_A.
  // Both kinds of tasks write FIELD_A and have ATOMIC coherence.
  //
  for(int i = 0; i < 100; i++) {
    inc_launcher_field_both.argument = TaskArgument(&i,sizeof(i));
    runtime->execute_task(ctx, inc_launcher_field_both);
  }
  for(int i = 0; i < 100; i++) {
    inc_launcher_fielda_only.argument = TaskArgument(&i,sizeof(i));
    runtime->execute_task(ctx, inc_launcher_fielda_only);
  }


  //
  // Sum up the answers.  Since we are just adding numbers up in different orders, the result
  // should always be the same.
  //
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

void inc_task_fielda_only(const Task *task,
			  const std::vector<PhysicalRegion> &regions,
			  Context ctx, Runtime *runtime)
{
  printf("Executing increment task %d for field A\n", *((int *) task->args));

  RegionAccessor<AccessorType::Generic, int> acc =
    regions[0].get_field_accessor(FIELD_A).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      int val = acc.read(DomainPoint::from_point<1>(pir.p));
      acc.write(DomainPoint::from_point<1>(pir.p), val + 1);
    }
}

void inc_task_fieldb_only(const Task *task,
			  const std::vector<PhysicalRegion> &regions,
			  Context ctx, Runtime *runtime)
{
  printf("Executing increment task %d for field B\n", *((int *) task->args));


  RegionAccessor<AccessorType::Generic, int> acc =
    regions[0].get_field_accessor(FIELD_B).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      int val = acc.read(DomainPoint::from_point<1>(pir.p));
      acc.write(DomainPoint::from_point<1>(pir.p), val + 1);
    }
}


void inc_task_field_both(const Task *task,
			 const std::vector<PhysicalRegion> &regions,
			 Context ctx, Runtime *runtime)
{
  printf("Executing increment task %d for field A += B\n", *((int *) task->args));

  RegionAccessor<AccessorType::Generic, int> acca =
    regions[0].get_field_accessor(FIELD_A).typeify<int>();

  RegionAccessor<AccessorType::Generic, int> accb =
    regions[1].get_field_accessor(FIELD_B).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      int valb = accb.read(DomainPoint::from_point<1>(pir.p));
      int vala = acca.read(DomainPoint::from_point<1>(pir.p));
      acca.write(DomainPoint::from_point<1>(pir.p), vala + valb);
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
  Runtime::register_legion_task<inc_task_fielda_only>(INC_TASK_ID_FIELDA,
                                                      Processor::LOC_PROC, 
                                                      true/*single launch*/, 
                                                      false/*no multiple launch*/);
  Runtime::register_legion_task<inc_task_fieldb_only>(INC_TASK_ID_FIELDB,
                                                      Processor::LOC_PROC, 
                                                      true/*single launch*/, 
                                                      false/*no multiple launch*/);
  Runtime::register_legion_task<inc_task_field_both>(INC_TASK_ID_BOTH,
                                                     Processor::LOC_PROC, 
                                                     true/*single launch*/, 
                                                     false/*no multiple launch*/);
  Runtime::register_legion_task<sum_task>(SUM_TASK_ID,
                                          Processor::LOC_PROC, 
                                          true/*single launch*/, 
                                          false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
