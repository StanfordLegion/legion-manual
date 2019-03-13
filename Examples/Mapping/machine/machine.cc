#include <cstdio>
#include "legion.h"
#include "default_mapper.h"

using namespace Legion;
using namespace Legion::Mapping;

// All tasks must have a unique task id (a small integer).
// A global enum is a convenient way to assign task ids.
enum TaskID {
  TOP_LEVEL_TASK_ID,
};

void top_level_task(const Task *task,
		    const std::vector<PhysicalRegion> &regions,
		    Context ctx, 
		    Runtime *runtime)
{
  printf("Running top level task...\n");
}

class MachineMapper : public DefaultMapper {
public:
  MachineMapper(MapperRuntime *rt, Machine m, Processor p);
public:
  static void register_machine_mappers(Machine machine, Runtime *rt,
                                       const std::set<Processor> &local_procs);
};

MachineMapper::MachineMapper(MapperRuntime *rt, Machine m, Processor p)
  : DefaultMapper(rt, m, p)
{
  // Find all processors of the same kind on the local node
  Machine::ProcessorQuery proc_query(m);
  // First restrict to the same node
  proc_query.local_address_space();
  // Make it the same processor kind as our processor
  proc_query.only_kind(p.kind());
  for (Machine::ProcessorQuery::iterator it = proc_query.begin(); 
        it != proc_query.end(); it++)
  {
    // skip ourselves
    if ((*it) == p)
      continue;
    printf("Mapper %s: shares " IDFMT "\n", get_mapper_name(), it->id);
  }
  // Find all the memories that are visible from this processor
  Machine::MemoryQuery mem_query(m);
  // Find affinity to our local processor
  mem_query.has_affinity_to(p);
  for (Machine::MemoryQuery::iterator it = mem_query.begin();
        it != mem_query.end(); it++)
    printf("Mapper %s: has affinity to memory " IDFMT "\n", get_mapper_name(), it->id);
}

/*static*/
void MachineMapper::register_machine_mappers(Machine machine, Runtime *rt,
                                             const std::set<Processor> &local_procs)
{
  printf("Replacing default mappers with custom mapper A on all processors...\n");
  MapperRuntime *const map_rt = rt->get_mapper_runtime();
  for (std::set<Processor>::const_iterator it = local_procs.begin();
       it != local_procs.end(); it++)
    {
      rt->replace_default_mapper(new MachineMapper(map_rt, machine, *it), *it);
    }
}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  {
    TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level_task");
    registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
    Runtime::preregister_task_variant<top_level_task>(registrar);
  }
  Runtime::add_registration_callback(MachineMapper::register_machine_mappers);

  return Runtime::start(argc, argv);
}
