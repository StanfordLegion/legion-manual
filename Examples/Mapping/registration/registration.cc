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

class CustomMapperA : public DefaultMapper {
public:
  CustomMapperA(MapperRuntime *rt, Machine m, Processor p)
    : DefaultMapper(rt, m, p) { }
public:
  static void register_custom_mappers(Machine machine, Runtime *rt,
                                      const std::set<Processor> &local_procs);
};

/*static*/
void CustomMapperA::register_custom_mappers(Machine machine, Runtime *rt,
                                            const std::set<Processor> &local_procs)
{
  printf("Replacing default mappers with custom mapper A on all processors...\n");
  MapperRuntime *const map_rt = rt->get_mapper_runtime();
  for (std::set<Processor>::const_iterator it = local_procs.begin();
       it != local_procs.end(); it++)
    {
      rt->replace_default_mapper(new CustomMapperA(map_rt, machine, *it), *it);
    }
}

class CustomMapperB : public DefaultMapper {
public:
  CustomMapperB(MapperRuntime *rt, Machine m, Processor p)
    : DefaultMapper(rt, m, p) { }
public:
  static void register_custom_mappers(Machine machine, Runtime *rt,
                                      const std::set<Processor> &local_procs);
};

/*static*/
void CustomMapperB::register_custom_mappers(Machine machine, Runtime *rt,
                                            const std::set<Processor> &local_procs) 
{
  printf("Adding custom mapper B for all processors...\n");
  MapperRuntime *const map_rt = rt->get_mapper_runtime();
  for (std::set<Processor>::const_iterator it = local_procs.begin();
       it != local_procs.end(); it++)
    {
      rt->add_mapper(1/*MapperID*/, new CustomMapperA(map_rt, machine, *it), *it);
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
  Runtime::add_registration_callback(CustomMapperA::register_custom_mappers);
  Runtime::add_registration_callback(CustomMapperB::register_custom_mappers);

  return Runtime::start(argc, argv);
}
