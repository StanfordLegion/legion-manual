
.. _chap:tasks:

Tasks
*****

All Legion programs are composed of *tasks* which are distinguished
functions that are used to control the granularity of parallelism.
Each Legion program is started by :ref:`creating a top-level task <sec:toplevel>`.
The top-level task is effectively the "main" function for a Legion program
where execution begins. Other than having a special mechanism to start
it, each top-level task is no different than any other task in a Legion
program. Tasks in a Legion program can launch :ref:`subtasks <sec:subtasks>` and 
:ref:`other operations <chap:operations>` in order to perform computations.
There is no Legion-imposed limit to the depth of tree of tasks for Legion 
programs [#f1]_. A Legion program exits when a top-level task and all its
descendent tasks and operations have completed execution.

Legion is an implicitly parallel programming model in which parallelism
is inferred automatically by the runtime using a dynamic program analysis.

.. note::
  The tree-based structure of tasks in a Legion programs mirrors the function 
  call tree of a normal sequential program written in any language. Therefore
  there exists a natural sequential semantics to every Legion program which
  the Legion runtime will always maintain.

Legion users are never required to explicitly describe what tasks will
and will not run in parallel. :ref:`Mappers <chap:mapping>` are empowered with
the ability to 
:ref:`decide whether to exploit or restrict the task parallelism <sec:maptasks>`
that the Legion runtime uncovers from the application program, but no such
considerations should be made when specifying the 
:ref:`algorithmic description <sec:mechfrompolicy>` of the program to Legion. 

.. _sec:subtasks:

Subtasks
========

When it comes to launching subtasks, it is up to users to determine which
functions should be promoted to being tasks and which should remain as
standard function invocations in the source language. The selection of 
which functions are to be made tasks will determine the granularity of 
parallelism that can be scheduled by Legion. Annotating too few functions
as tasks can limit parallelism and make it challenging for Legion to 
overlap computation with communication. Annotating too many functions as
tasks can result in excessively high runtime overhead as Legion will
struggle to ammortize the cost of its dynamic analysis with actual
application execution. As a general rule of thumb, we encourage users to 
aim for annotating *inner tasks* (functions that launch subtasks) that require
between 10-100 microseconds of execution time, and *leaf tasks* (functions
that do not launch subtasks) to have around 100-1000 microseconds of latency.
These should be treated as rough guidelines, and can also be reduced when
using :ref:`other features of Legion <chap:tracing>`. We also provide 
:ref:`tools <sec:prof>` for identifying when task granularities are not 
appropriately tuned.

Subtasks are created by constructing a ``TaskLauncher`` object with the
parameters describing the subtask and invoking the ``execute_task`` method:

.. code-block:: c++

  struct TaskLauncher {
  public:
    TaskLauncher(void);
    TaskLauncher(TaskID tid, 
                 TaskArgument arg,
                 Predicate pred = Predicate::TRUE_PRED,
                 MapperID id = 0,
                 MappingTagID tag = 0);
  public:
    inline IndexSpaceRequirement&
            add_index_requirement(const IndexSpaceRequirement &req);
    inline RegionRequirement&
            add_region_requirement(const RegionRequirement &req);
    inline void add_field(unsigned idx, FieldID fid, bool inst = true);
  public:
    inline void add_future(Future f);
    inline void add_grant(Grant g);
    inline void add_wait_barrier(PhaseBarrier bar);
    inline void add_arrival_barrier(PhaseBarrier bar);
    inline void add_wait_handshake(LegionHandshake handshake);
    inline void add_arrival_handshake(LegionHandshake handshake);
  public:
    inline void set_predicate_false_future(Future f);
    inline void set_predicate_false_result(TaskArgument arg);
  public:
    inline void set_independent_requirements(bool independent);
  public:
    TaskID                             task_id;
    std::vector<IndexSpaceRequirement> index_requirements;
    std::vector<RegionRequirement>     region_requirements;
    std::vector<Future>                futures;
    std::vector<Grant>                 grants;
    std::vector<PhaseBarrier>          wait_barriers;
    std::vector<PhaseBarrier>          arrive_barriers;
    TaskArgument                       argument;
    Predicate                          predicate;
    MapperID                           map_id;
    MappingTagID                       tag;
    DomainPoint                        point;
    IndexSpace                         sharding_space;
  public:
    Future                             predicate_false_future;
    TaskArgument                       predicate_false_result;
  public:
    const std::vector<StaticDependence> *static_dependences;
  public:
    bool                               enable_inlining;
  public:
    bool                               local_function_task;
  public:
    bool                               independent_requirements;
  public:
    bool                               silence_warnings;
  };

  Future Runtime::execute_task(Context ctx, const TaskLauncher &launcher,
                          std::vector<OutputRequirement> *outputs = NULL);

Each task is created with a ``task_id`` that uniquely names the function
to be performed by the task. Note that users do not actually name the 
impelemntation of this function. Instead, subtask launches come equipped
with a dynamic dispatch mechnaism that will allow the owning mapper
(specified by ``map_id`` in the launcher) to select which *variant* of
the task to implement. A :ref:`task variant <sec:taskvariants>` is a specific
implementation of a task that is customized with a particular algorithm
for implementation that function or for a particular class of hardware
(or both). Users can generated fresh ``TaskID`` values 
:ref:`statically <subsec:preresource>`, :ref:`for libraries <sec:registrationcallbacks>`,
or dynamically by invoking the ``generate_dynamic_task_id`` method at any time.

.. code-block:: c++

  TaskID Runtime::generate_dynamic_task_id(void); 

The ``index_requirements`` and ``region_requirements`` vectors allow the
application to specify the requested *privileges* the :ref:`will be needed to
execute the subtask <sec:reqs>`. The vector ``futures`` passes in 
:ref:`future <sec:futures>` values that will be consumed by the subtask. The
``grants``, ``wait_barriers``, and ``arrive barriers`` provide 
:ref:`non-canonical synchronization mechanisms for tasks <chap:noncanonical>`
and are therefore infrequently used. The ``argument`` members describes
a ``TaskArgument`` object, which is a lightweight wrapper around a pointer
and stride for passing a buffer by value to the subtask.

.. code-block:: c++

  class TaskArgument : public Unserializable<TaskArgument> {
  public:
    TaskArgument(void) : args(NULL), arglen(0) { }
    TaskArgument(const void *arg, size_t argsize)
      : args(const_cast<void*>(arg)), arglen(argsize) { }
    TaskArgument(const TaskArgument &rhs)
      : args(rhs.args), arglen(rhs.arglen) { }
  public:
    inline size_t get_size(void) const { return arglen; }
    inline void*  get_ptr(void) const { return args; }
  public:
    inline bool operator==(const TaskArgument &arg) const
      { return (args == arg.args) && (arglen == arg.arglen); }
    inline bool operator<(const TaskArgument &arg) const
      { return (args < arg.args) && (arglen < arg.arglen); }
    inline TaskArgument& operator=(const TaskArgument &rhs)
      { args = rhs.args; arglen = rhs.arglen; return *this; }
  private:
    void *args;
    size_t arglen;
  }; 

The ``TaskArgument`` class has two members: ``args`` which describes
a ``void*`` pointer to the buffer to be passed, and ``arglen`` describing
the size of the buffer in bytes. Note that Legion does not copy this 
buffer upon the creation of the ``TaskLauncher`` or upon the creation
of the ``TaskArgument`` object. Only when the call to the runtime is
invoked, in this case ``execute_task`` is made will the buffer be copied.
This pattern is repeated throughout the API design.

The ``predicate`` member of the ``TaskLauncher`` describes a *predicate value*
that controls where the task is ultimately executed or not via 
:ref:`predicated execution <sec:predication>`. By default the ``predicate`` value
is set a ``TRUE_PRED`` value indicating that the task will always run. In
the case that ``predicate`` value evaluates to ``FALSE_PRED`` and the task
is not run, the application can specify an alternative value to use for 
setting the ``Future`` returned from ``execute_task``. This can be done
by specifying either a ``predicate_false_future`` or a pass-by-value
``TaskArgument`` on ``predicate_false_value``. If both are set, the runtime
will give priority to ``predicate_false_future``. Note again that 
``predicate_false_value`` is only copied upon the invocation of ``execute_task``.

The ``map_id`` member names the :ref:`kind of mapper <chap:mapping>` that will be
responsible for :ref:`handling mapping queries needed for the execution of 
the subtask <sec:maptasks>`. The ``mapping_tag`` is an ``unsigned int`` that
the runtime passes by value uninspected through to any mapper calls made 
to map the subtask. ``MappingTagID`` arguments permit the application to 
provide domain-specific context to the mapper about the launch of a subtask
so that the mapper can better tailor its mapping decisions. It is the 
perrogative of the application and the mapper to determine whether and how
``MappingTagID`` values are utilized.

The ``point`` member of the ``TaskLauncher`` describes a type-erase ``DomainPoint``.
The application can set this value arbitrarily to help determine how best to
*shard* this task for :ref:`control replication <chap:ctrlrepl>`. The ``sharding_space``
member also provides an argument solely to :ref:`sharding functors <sec:sharding>`
for the purpose of *sharding* subtask execution.

The ``static_dependences`` member of ``TaskLauncher`` allows applications to
specify known compile-time dependences on previous task launches via
``StaticDependence`` descriptors. We cover how static dependences are 
specified when we describe :ref:`static tracing <sec:statictracing>`.



.. _sec:contexts:

Contexts
========

.. _sec:futures:

Futures
=======

.. _sec:threading:

Execution Model
===============

.. tls, preemption

.. _sec:predication:

Predicated Execution
====================

.. _sec:inlining:

Inlined Execution
=================

.. rubric:: Footnotes

.. [#f1] Programs can still run out of other resources if they have a task tree that is too deep such as stack space or heap memory, but this error will not come from Legion.
