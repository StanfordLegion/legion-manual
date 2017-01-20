#include <cstdio>
#include "legion.h"
#include "default_mapper.h"

using namespace LegionRuntime::HighLevel;
using namespace LegionRuntime::Accessor;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
  SUBTASK_PRODUCER_ID,
  SUBTASK_CONSUMER_ID,
};

enum FieldIDs {
  FIELD_A,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    HighLevelRuntime *runtime)
{

  printf("Top level task start.\n");
  Rect<1> rec(Point<1>(0),Point<1>(99));
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

  for(int i = 1; i <= 100; i += 2) {
    TaskLauncher producer_launcher(SUBTASK_PRODUCER_ID, TaskArgument(&i,sizeof(int)));
    Future doubled_task_number =  runtime->execute_task(ctx,producer_launcher);
    TaskLauncher consumer_launcher(SUBTASK_CONSUMER_ID, TaskArgument(NULL,0));
    consumer_launcher.add_future(doubled_task_number);
    consumer_launcher.add_region_requirement(crr);
    runtime->execute_task(ctx,consumer_launcher);
  }
  printf("Top level task done launching subtasks.\n");
}

int subtask_producer(const Task *task,
		      const std::vector<PhysicalRegion> &regions,
		      Context ctx, 
		      HighLevelRuntime *runtime)
{
  int subtask_number = *((int *) task->args);
  printf("\tProducer subtask %d\n", subtask_number);
  return subtask_number + 1;
}

void subtask_consumer(const Task *task,
		     const std::vector<PhysicalRegion> &regions,
		     Context ctx, 
		     HighLevelRuntime *runtime)
{
  Future f = task->futures[0];
  int subtask_number = f.get_result<int>();
  printf("\tConsumer subtask %d\n", subtask_number);

  RegionAccessor<AccessorType::Generic, int> acc =
    regions[0].get_field_accessor(FIELD_A).typeify<int>();

  Domain dom = runtime->get_index_space_domain(ctx,
					       task->regions[0].region.get_index_space());
  Rect<1> rect = dom.get_rect<1>();
  for (GenericPointInRectIterator<1> pir(rect); pir; pir++)
    {
      int val = acc.read(DomainPoint::from_point<1>(pir.p));
      acc.write(DomainPoint::from_point<1>(pir.p), val + subtask_number);
    }
}

class RoundRobinMapper : public DefaultMapper {
public:
  RoundRobinMapper(Machine machine,
		    HighLevelRuntime *rt, Processor local);
  virtual void select_task_options(Task *task);
//  virtual void slice_domain(const Task *task, const Domain &domain,
//                            std::vector<DomainSplit> &slices);
  virtual bool map_task(Task *task);
//  virtual void notify_mapping_result(const Mappable *mappable);
private:
  int next_proc;
};

RoundRobinMapper::RoundRobinMapper(Machine m,
				   HighLevelRuntime *rt, Processor p)
  : DefaultMapper(m, rt, p) // pass arguments through to DefaultMapper                                                                                                                                
{
  next_proc = 0;
}

void mapper_registration(Machine machine, HighLevelRuntime *rt,
			 const std::set<Processor> &local_procs)
{
  for (std::set<Processor>::const_iterator it = local_procs.begin();
       it != local_procs.end(); it++)
    {
      rt->replace_default_mapper(new RoundRobinMapper(machine, rt, *it), *it);
    }
}

void RoundRobinMapper::select_task_options(Task *task)
{
  task->inline_task = false;
  task->spawn_task = false;
  task->map_locally = false;
  task->profile_task = false;
  task->task_priority = 0;

  std::set<Processor> all_procs;
  machine.get_all_processors(all_procs);
  std::vector<Processor> valid_options;

  for (std::set<Processor>::const_iterator it = all_procs.begin(); it != all_procs.end(); it++) {
    if (it->kind() == Processor::LOC_PROC) {
      valid_options.push_back(*it);
    }
  }
  if (!valid_options.empty()) {
    if (valid_options.size() > next_proc) {
      task->target_proc = valid_options[next_proc++];
    }
    else {
      task->target_proc = valid_options[0];
      next_proc = 1;
    }
  }
  else
    task->target_proc = Processor::NO_PROC;
}

bool RoundRobinMapper::map_task(Task *task)
{
  //  std::set<Memory> vis_mems;
  //  machine.get_visible_memories(task->target_proc, vis_mems);
  //  assert(!vis_mems.empty());
  for (unsigned idx = 0; idx < task->regions.size(); idx++)
    {
      if (task->regions[idx].current_instances.size() > 0) {
	assert(task->regions[idx].current_instances.size() == 1);
	task->regions[idx].target_ranking.push_back(
						    (task->regions[idx].current_instances.begin())->first);
      }
      else
	{
	  Memory global = machine_interface.find_global_memory();
	  assert(global.exists());
	  task->regions[idx].target_ranking.push_back(global);
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
  HighLevelRuntime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  HighLevelRuntime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
						   Processor::LOC_PROC, 
						   true/*single launch*/, 
						   false/*no multiple launch*/);
  HighLevelRuntime::register_legion_task<int,subtask_producer>(SUBTASK_PRODUCER_ID,
						      Processor::LOC_PROC, 
						      true/*single launch*/, 
						      false/*no multiple launch*/);
  HighLevelRuntime::register_legion_task<subtask_consumer>(SUBTASK_CONSUMER_ID,
						  Processor::LOC_PROC, 
						  true/*single launch*/, 
						  false/*no multiple launch*/);

  HighLevelRuntime::set_registration_callback(mapper_registration);

  return HighLevelRuntime::start(argc, argv);
}
