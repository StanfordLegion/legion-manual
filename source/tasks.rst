
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

The ``enable_inlining`` flag indicates to the runtime whether this task should
be eligible for :ref:`inlining <sec:inlining>` in the parent task. The mapper will
ultimately given the choice of whether to inline the task or not as part of
its dynamic dispatch mechanism. However, there is a small performance penalty
associated with giving the mapper this option as execution must wait for the
mapper to make this decision. Therefore applications must explicitly opt into
giving the mapper the choice to inline a subtask launch. To avoid this overhead
in the common case, this flag defaults to ``false``.

The ``local_function`` flag indicates to the runtime that this is a subtask
operating solely on future values with no region arguments. This permits a
very light-weight execution strategy for the task and allows it to be trivially
replicated if the parent task is :ref:`control replicated <chap:ctrlrepl>`.
This is only a performance optimization and has no bearning on the correctness
of the code. The default value for this flag is ``false``.

The ``independent_requirements`` flag allows applications to communicate to
the runtime that the all of the :ref:`region requirements <sec:reqs>` of this
task launch are independent from each other. This means that they are have
no :ref:`fields <sec:fieldspaces>` in common, access different 
:ref:`region trees <sec:logicalregions>`, or their subregions are strictly
*disjoint* (non-overlapping) from each other. Region requirements which are
overlapping but have non-interfering :ref:`privileges <sec:modes>` are not
permitted. Setting this flag to ``true`` will enable several performance
optimizations inside the runtime implementation that will reduce the overhead
of the dependence analysis for this task. Undefined behavior will result if
this flag is set to ``true``, but the regions are not actually independent.
The runtime has no mechanism by which to detect the violation. The default
value for this flag is ``false``.

The ``silence_warnings`` flag can be set to ``true`` in order to silence any
runtime warnings associated with this task. This is useful in cases where
users know that in some circumstances warnings can be safely ignored, thereby
filtering out the warning messages to arrive at the most important ones. This
flag defaults to ``false``.

After an application has configured a ``TaskLauncher`` object for a subtask
execution. The subtask is dispatched using the ``execute_task`` method on
the ``Runtime``. Users must pass in a :ref:`Context <sec:contexts>` handle
specific to the parent task. Users can also pass in an optional pointer to
a vector of ``OutputRequirement`` objects which :ref:`describe new regions
that will be created by the task <sec:outputreqs>`. Except for in the case
of :ref:`inlining <sec:inlining>`, this method will return immediately as
all subtask launches in Legion are performed asynchonously. All contents of
the ``TaskLauncher`` object are needed to execute the subtask are copied 
by the runtime by the implementation of ``execute_task``, so at the completion
of the method invocation, it is safe to re-use the launcher for additional
subtask launches. The ``const`` qualifier on the ``TaskLauncher`` reference
guarantees that the ``execute_task`` implementation does not modify the
``TaskLauncher``. Each dynamic invocation of ``execute_task`` with a 
``TaskLauncher`` object will produce a new dynamic task at execution. The
return value of ``execute_task`` is a ``Future``. :ref:`Futures <sec:futures>`
encapsulate the return value of the task and also provide a mechanism for
synchronizing subtask execution if descired. ``Future`` handles are always
returned for tasks, even those with ``void`` return types as the future can
still be useful for synchonization purposes.

.. _sec:contexts:

Contexts
========

``Context`` objects are opaque light-weight handles that provide the runtime
an easy mechanism for recognizing which parent task *context* runtime calls
are being performed in. When a task begins executing, the runtime provides a
``Context`` handle as the first argument to the task. This handle should then
be used for all method calls into the ``Runtime`` that require a ``Context``
argument (nearly always the first argument in such methods). ``Context``
handles are inexpensive to pass-by-value (usually only four or eight bytes
depending on the target architecture). Users should therefore feel no need
to pass them by reference or pass pointers to them. They will always be 
`trivially copyable <https://en.cppreference.com/w/cpp/types/is_trivially_copyable>`_.

The lifetime of a ``Context`` handle is only for the duration of the execution
of the task for which it was produced. Users should therefore never store
``Context`` values in data structures that persist beyond the execution of 
a task. Using a ``Context`` handle after the execution of the task for which 
it was created will result in undefined behavior.

In keeping with our :ref:`coarse-grained functional philosophy <sec:funcimperative>`,
we recommend that applications design functional abstractions that accept
``Context`` handles as arguments to pass through for launching off subtasks
and other operations. In essence, the ``Context`` handle is the 
`monad <https://wiki.haskell.org/Monad>`_ that specifies the functional 
order of subtasks and other operations being launched by a parent task, even
if they ultimately are executed in a different order. Therefore, passing it by
value and threading it through functions is consistent with how functional 
programming launguages encourage code development. To ease this burden though
for more imperative users, we also support a runtime method that will return
the ``Context`` handle for the executing task if we are inside of one:

.. code:: c++

  static Context Runtime::get_context(void);

If we are inside of an executing task, this will return an identical ``Context``
handle. If we are not inside of a task, then it will raise an error. Note that
this method is ``static`` and can therefore be invoked arbitrarily, anywhere
in a program. While we do not recommend this style of code development, it is
and always remaing completely legal to development Legion programs using it.

..  _sec:futures:

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
