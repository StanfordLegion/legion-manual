#include <cstdio>
#include "legion.h"

using namespace Legion;
using namespace LegionRuntime::Arrays;

enum TaskID {
  TOP_LEVEL_TASK_ID,
};

void print_array(coord_t *a, int dim) {
  printf("The point is <");
  for(int i = 0; i < dim-1; i++) {
    printf("%zd,",a[i]);
  }
  printf("%zd>\n",a[dim-1]);
}

void top_level_task(const Task *task,
                      const std::vector<PhysicalRegion> &regions,
                      Context ctx, HighLevelRuntime *runtime)
{
  //
  // Point operations
  //
  coord_t coords[3];
  coord_t source[3];

  Point<1> one(1);
  one.to_array(coords);
  print_array(coords,1);

  Point<1> two(2);
  two.to_array(coords);
  print_array(coords,1);

  (one + two).to_array(coords);
  print_array(coords,1);

  //  (one * two).to_array(coords);
  //  print_array(coords,1);

  source[0] = 0;
  source[1] = 0;
  Point<2> zero(source);
  zero.to_array(coords);
  print_array(coords,2);

  Point<1> anotherone = make_point(1);
  Point<2> zeroes = make_point(0,0);
  Point<2> twos = make_point(2,2);
  Point<2> threes = make_point(3,3);
  Point<3> fours = make_point(4,4,4);

  printf("The point is <%zd>\n",anotherone[0]);
  printf("The point is <%zd,%zd>\n",twos[0],twos[1]);
  printf("The point is <%zd,%zd>\n",threes[0],threes[1]);
  printf("The point is <%zd,%zd,%zd>\n",fours[0],fours[1],fours[2]);
  (twos + threes).to_array(coords);
  print_array(coords,2);
  (twos * threes).to_array(coords);
  print_array(coords,2);
  printf("The dot product is %zd\n",twos.dot(threes));
  
  assert(twos == twos);
  assert(twos != threes);
  assert(twos <= threes);

  //
  // Rect operations
  //
  Rect<2> big(zeroes,threes);
  Rect<2> small(twos,threes);
  assert(big != small);
  assert(big.contains(small));
  assert(small.overlaps(big));
  assert(small.convex_hull(big) == big);
  assert(small.intersection(big) == small);

  //
  // Conversion to Domain and DomainPoint
  //
  Domain bigdomain = Domain::from_rect<2>(big);
  DomainPoint dtwos = DomainPoint::from_point<2>(twos);

  // useless statements to suppress warnings about unused variables
  bigdomain = bigdomain;
  dtwos = dtwos;

}

int main(int argc, char **argv)
{
  Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
  Runtime::register_legion_task<top_level_task>(TOP_LEVEL_TASK_ID,
                                                Processor::LOC_PROC, 
                                                true/*single launch*/, 
                                                false/*no multiple launch*/);
  return Runtime::start(argc, argv);
}
