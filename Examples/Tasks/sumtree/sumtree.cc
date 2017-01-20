#include <cstdio>
#include "legion.h"

using namespace LegionRuntime::HighLevel;
enum TaskID {
  SUM_ID,
  SUM_TREE_ID,
};

int arg_size = 2 * sizeof(int);

//
// Dynamically allocates and returns a pointer to an array of two integers.
//
int *sum_arg(int low, int high) {
  int *range = (int *) malloc(arg_size);
  range[0] = low;
  range[1] = high;
  return range;
}

//
//  The top level task.  Takes an optional command line argument and
//  starts a summation tree.
//
void sum_task(const Task *task,
	      const std::vector<PhysicalRegion> &regions,
	      Context ctx, 
	      HighLevelRuntime *runtime)
{
  int high = 1000;
  const InputArgs &command_args = HighLevelRuntime::get_input_args();
  if (command_args.argc > 1)
    {
      high = atoi(command_args.argv[1]);
      assert(high >= 0);
    }
  printf("Computing the sum of 0 to %d\n", high);

  int *range = sum_arg(0,high);
  TaskLauncher launcher(SUM_TREE_ID, TaskArgument(range, arg_size));
  Future sum = runtime->execute_task(ctx, launcher);
  int result = sum.get_result<int>();
  free(range);
  printf("The sum is %d.\n",result);
}

  
//
//  The input is an array of two integers, the first of which is always less than or eqaul to the second.
//  Sums the range using the following algorithm:
//  If the range is [x,x], return x.
//  If the range is [x,y] where x < y, split the range in half, recursively sum the two halves,
//  and then sum the two results.
//
int sum_tree_task(const Task *task,
		  const std::vector<PhysicalRegion> &regions,
		  Context ctx, 
		  HighLevelRuntime *runtime) {
  int *range  = (int *) task->args;
  int low = range[0];
  int high = range[1];

  if (low == high)
    return low;

  int midpoint = low + (high - low) / 2;

  int *lowrange = sum_arg(low,midpoint);
  TaskLauncher lowlauncher(SUM_TREE_ID, TaskArgument(lowrange,arg_size));
  Future lowsum = runtime->execute_task(ctx, lowlauncher);

  int *highrange = sum_arg(midpoint+1,high);
  TaskLauncher highlauncher(SUM_TREE_ID, TaskArgument(highrange,arg_size));
  Future highsum = runtime->execute_task(ctx, highlauncher);

  int sum = lowsum.get_result<int>() + highsum.get_result<int>();

  printf("The partial sum of %d to %d is %d\n",low, high, sum);
  free(lowrange);
  free(highrange);
  return sum;
}

int main(int argc, char **argv)
{
  HighLevelRuntime::set_top_level_task_id(SUM_ID);
  HighLevelRuntime::register_legion_task<sum_task>(SUM_ID,
						   Processor::LOC_PROC, 
						   true/*single launch*/, 
						   false/*no multiple launch*/);
  HighLevelRuntime::register_legion_task<int,sum_tree_task>(SUM_TREE_ID,
							Processor::LOC_PROC, 
							true/*single launch*/, 
							false/*no multiple launch*/);

  return HighLevelRuntime::start(argc, argv);
}
