#include <cstdio>
#include "legion.h"
#include "default_mapper.h"

using namespace Legion;
using namespace LegionRuntime::Accessor;
using namespace LegionRuntime::Arrays;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  TASK_INC,
};

enum FieldIDs {
  FIELD_A,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{

  printf("Top level task start.\n");
  Rect<1> rec(Point<1>(0),Point<1>(9999));
  IndexSpace sis = runtime->create_index_space(ctx,Domain::from_rect<1>(rec));
  FieldSpace fs = runtime->create_field_space(ctx);
  FieldAllocator field_allocator = runtime->create_field_allocator(ctx,fs);
  FieldID fida = field_allocator.allocate_field(sizeof(int), FIELD_A);
  assert(fida == FIELD_A);

  LogicalRegion lr = runtime->create_logical_region(ctx,sis,fs);

  int init = 1;
  runtime->fill_field(ctx,lr,lr,fida,&init,sizeof(init));
  
  RegionRequirement crr(lr, READ_WRITE, ATOMIC, lr);
  crr.add_field(FIELD_A);

  for(int i = 1; i <= 100; i++) {
    TaskLauncher inclauncher(TASK_INC, TaskArgument(&i,sizeof(int)));
    inclauncher.add_region_requirement(crr);
    runtime->execute_task(ctx,inclauncher);
  }
  printf("Top level task done launching subtasks.\n");
}

void inc_task(const Task *task,
	      const std::vector<PhysicalRegion> &regions,
	      Context ctx, 
	      Runtime *runtime)
{
  printf("\tInc task %d\n", *((int *) task->args));

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

class RoundRobinMapper : public DefaultMapper {
public:
  RoundRobinMapper(Machine machine,
		    Runtime *rt, Processor local);
  //  virtual void select_task_options(Task *task);
//  virtual void slice_domain(const Task *task, const Domain &domain,
//                            std::vector<DomainSplit> &slices);
  virtual bool map_task(Task *task);
//  virtual void notify_mapping_result(const Mappable *mappable);
private:
  int next_proc;
};

RoundRobinMapper::RoundRobinMapper(Machine m,
				   Runtime *rt, Processor p)
  : DefaultMapper(m, rt, p) // pass arguments through to DefaultMapper                                                                                                                                
{
  next_proc = 0;
}

void mapper_registration(Machine machine, Runtime *rt,
			 const std::set<Processor> &local_procs)
{
  for (std::set<Processor>::const_iterator it = local_procs.begin();
       it != local_procs.end(); it++)
    {
      rt->replace_default_mapper(new RoundRobinMapper(machine, rt, *it), *it);
    }
}

bool RoundRobinMapper::map_task(Task *task)
{
  
  for (unsigned idx = 0; idx < task->regions.size(); idx++)
    {
      if (task->regions[idx].current_instances.size() > 0) {
	assert(task->regions[idx].current_instances.size() == 1);
	printf("mapping to memory containing current unique instance\n");
	task->regions[idx].target_ranking.push_back(
						    (task->regions[idx].current_instances.begin())->first);
      }
      else
	{
	  Memory global = machine_interface.find_global_memory();
	  assert(global.exists());
	  task->regions[idx].target_ranking.push_back(global);
	  printf("mapping to global memory.\n");
	}
      task->regions[idx].virtual_map = false;
      task->regions[idx].enable_WAR_optimization = false;
      task->regions[idx].reduction_list = false;
      task->regions[idx].blocking_factor = 1;
    }
  // Report successful mapping results                                                                                                                                                                
  return true;
}


int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  Runtime::register_legion_task<inc_task>(TASK_INC,
                                          Processor::LOC_PROC, 
                                          true/*single launch*/, 
                                          false/*no multiple launch*/,
                                          AUTO_GENERATE_ID,
                                          TaskConfigOptions(true,false,false)
                                          );

  Runtime::set_registration_callback(mapper_registration);

  return Runtime::start(argc, argv);
}
