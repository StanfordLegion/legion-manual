
.. _chap:boot:

Bootstrapping
*************

Regardless of a user's choice of language for writing Legion
programs, all Legion programs must first begin by bootstrapping
the runtime. Legion is designed to work in both single-process and
multi-process environments, with multi-process being the much more
common deployment methodology. Legion is almost entirely ambivalent 
about how these processes are created. In single-process execution,
the program can be started as any other. For multi-process execution,
we recommend using `mpirun <https://www.open-mpi.org/doc/v4.0/man1/mpirun.1.php>`_
as a highly scalable and portable way to create these processes. Users
are also welcome to use any of the GASNet supported spawners. For
the most recent documantation on these spawners please visit the 
`GASNet website <https://gasnet.lbl.gov/>`_. The only constraints
that Legion requires for multi-process execution, are the ones
enforced upon it by GASNet (e.g. the presence of a 
`PMI implementation <https://wiki.mpich.org/mpich/index.php/PMI_v2_API>`_). 

Once processes are running, the application needs to startup the Legion runtime
inside of these processes. We assume that prior to starting the Legion
runtime, these processes are operating in an 
`SPMD execution style <https://en.wikipedia.org/wiki/SPMD>`_ where
each process will be making the same sequence of API calls to configure
the Legion runtime prior to the call to start the runtime [#f1]_. If a program
violates this basic principle, then undefined behavior will result. We
also assume that there will be exactly one thread bootstrapping the 
Legion runtime in each process. If a program violates this principle, undefined
behavior will result. Often this thread is the same thread that executes
``main`` but we do not require that they be the same thread.
The rest of this chapter will detail the various stages that occur
when starting the Legion runtime in execution order.

.. _sec:preregistration:

Pre-Registration
=====================

Before starting the Legion runtime, there are a number of static
API calls that users can invoke on the ``Runtime`` class to register
various kinds of resources with the runtime or to configure the
runtime for execution prior to starting it. We refer to this stage
of the execution as the *preregistration* phase of execution. As
mentioned earlier (and we cannot stress this enough), all preregistration
calls must be performed from a single thread and in the same order
across all processes or undefined behavior will result. However, 
Legion's implementation of preregistration calls are safe to be
performed prior to the ``main`` function being invoked. Therefore
dynamic libraries which have code that run as part of being invoked
by `dlopen <https://man7.org/linux/man-pages/man3/dlopen.3.html>`_
prior to ``main`` being called are safe to invoke the Legion 
runtime's static preregistration methods. Users are responsibile 
though for ensuring that such dynamic objects are loaded in the same 
order across all the processes, which should occur naturally if the 
same binary is running in each process. 

Preregistration calls fall into two categories: configuration of
runtime options and registration of resource objects to be used by
Legion programs. 

.. _subsec:preconfig:

Runtime Configuration
---------------------

There are several optional runtime configuration calls that can be 
performed before start-up of the Legion runtime. First, is the static 
method to set the task ID of the top-level task to be dispatched by the 
runtime upon the completion of start-up.

.. code-block:: c++

  static void Runtime::set_top_level_task_id(TaskID task_id);

This method can be called repeatedly, but the value set by the 
last invocation will be the one task actually dispatched. Note that this
is a legacy (but not deprecated) method as there are
:ref:`other ways to dispatch top-level tasks after starting the runtime <sec:toplevel>`.
In addition to configuring the top-level task to run, users can also 
specify the ID of the mapper to use for mapping the top-level task.

.. code-block:: c++

  static void Runtime::set_top_level_task_mapper_id(MapperID mapper_id);

This method can also be called repeatedly and only the last value will
be used. Again, this is more of a legacy function for programs that 
will launch only a single top-level task, but is not deprecated.

Perhaps the most important configuration method is the one that 
allows users to register callbacks from the runtime for further
registration of resources after the runtime has started, but before
any top-level tasks are permitted to begin running. We refer to 
functions as *registration callbacks*. All registration callbacks
must be function pointers with the following type signature:

.. code-block:: c++

  typedef void (*RegistrationCallbackFnptr)(Machine machine, 
                                            Runtime* runtime, 
                                            const std::vector<Processor>& procs);  

Function pointers are registered with the Legion runtime using this method:

.. code-block:: c++
  
  static void Runtime::add_registration_callback(RegistrationCallbackFnptr fnptr);

After the runtime has been fully initialized, but before any top-level
tasks have run, then the runtime will proceed to invoke each of these
callbacks in the order in which they were added (FIFO). We discuss 
:ref:`the semantics of these callbacks <sec:registrationcallbacks>` later.
A common idiom for Legion libraries is to record a registration callback, 
and then perform all their registrations once the runtime has started and
has more facilities to offer.

.. _subsec:preresource:

Resource Pre-Registration
-------------------------

The second class of preregistration functions are methods that are
used for registering resources such as task variants, reduction 
operations, etc. All of these resources can also be registered 
:ref:`after the runtime is started <sec:registrationcallbacks>`, 
but for applications that know these resources up-front, it can be 
useful to preregister these resources. 


.. _sec:startup:

Starting the Legion Runtime
===========================

The Legion runtime is started be invoking the ``start`` method:

.. code-block:: c++

  static int Runtime::start(int argc, 
                            char **argv, 
                            bool background = false, 
                            bool supply_default_mapper = true);

The start method should be invoked exactly once in each process. Multiple
invocations will result in undefined behavior. Even after waiting for the
runtime to shutdown, there is no safe way to restart it. The first two arguments
to this method are ``argc``, the number of arguments passed to ``main``, and
``argv``, the array of strings passed to ``main``. Legion will read these to look
for :ref:`command line arguments <sec:commandline>`  passed to Legion. The third
argument is a boolean indicating whether control should return immediately 
(if set to ``true``) or whether the runtime should put this thread to sleep until
all Legion top-level tasks are finished executing. The fourth argument indicates
whether Legion should supply an implementation of the :ref:`default mapper <sec:defaultmapper>`
for ``MapperID`` zero (see the chapter on :ref:`mapping <chap:mapping>` for more
information on ``MapperID``). In the case of ``background`` being set to ``true``, 
the return value of the function will be zero if the runtime succeeds in starting and non-zero 
otherwise. In the case of ``background`` being set to ``false``, the return value will
represent the last invocation of the ``set_return_code`` function:

.. code-block:: c++

  static void Runtime::set_return_code(int return_code);

The ``set_return_code`` function is a static method that can be called by any task
anywhere during the execution of the Legion runtime. Only the last invocation of
``set_return_code`` before the runtime shuts down will be reported.

If the runtime is run in the background of the thread that starts it, then it is up
to the application to call the following method before exiting the process.

.. code-block:: c++

  static int Runtime::wait_for_shutdown(void);

This method will block the calling thread, wait for all top-level tasks to finish
executing, and wait for the Legion runtime to shut itself down. This call should be
made exactly once per process. Multiple invocations will result in undefined behavior.
This call does not have to happen in the same thread that invoked ``start``, but there
does need to exist a happens-before relationship between the invocations of ``start``
and ``wait_for_shutdown``. No call to ``wait_for_shutdown`` is required if the
thread that called ``start`` set ``background`` to ``false``, but performing such a
call will succeed and will return with the same return value as ``start``. Exiting the
process before calling ``wait_for_shutdown`` when the runtime is executing in the background
will result in undefined behavior.

There also exists an optional helper method for network layers (such as MPI and GASNet) 
that may need to rewrite ``argc`` and ``argv`` before users can access them. Since these
calls to lower level networking layers are normally done implicitly inside the 
implementation of the ``start`` method, it may be unsafe for users to access
the command line arguments until after ``start`` has been called. To resolve this issue
Legion, also provides the ``initialize`` method:

.. code-block:: c++

  static void Runtime::initialize(int *argc, 
                                  char ***argv, 
                                  bool filter = false);

The initialize method should be invoked exactly once per process. Multiple invocations
per process will result in undefined behavior. The initialize method will perform the 
requisite calls to the underlying networking layer to rewrite the command line arguments, 
and return the resulting command line arguments back to the application without starting 
the Legion runtime. The call to ``initialize`` must occur before the call 
to ``start`` and may occur in different threads as long as there exists a happens-before
relationship between the two calls. The ``filter`` parameter to the call specifies whether
Legion should remove its command line arguments from the returned array of strings in ``argv``.
If ``filter`` is set to ``true``, then Legion will rewrite the command line arguments
to remove all :ref:`Legion command line arguments <sec:commandline>` from ``argv`` and
reduce the number of arguments represented by ``argc`` accordingly.

After the Legion runtime has been started, the following method for obtaining a pointer
to the ``Runtime`` instance is universally available until the runtime is shutdown:

.. code-block:: c++

  static Runtime* Runtime::get_runtime(Processor proc = Processor::NO_PROC);

Note that this is a static method and therefore can be invoked anywhere without any context 
in order to get the ``Runtime`` instance and then use it to perform runtime calls. Nominally, 
there is only one Legion runtime instance in each process, so the ``proc`` argument can 
usually be ignored. However, there are some configurations 
(see :ref:`command line arguments <sec:commandline>`) that will create multiple runtime 
instances in the same process, and the ``proc`` argument specifies which processor the 
runtime instance should be associated with. 

After the runtime is started, users can also universally obtain access to the command line
parameters passed into the Legion runtime using the following method:

.. code-block:: c++

  struct InputArgs {
    char **argv;
    int argc;
  };

  static const InputArgs& Runtime::get_input_args(void);

This method will return an ``InputArgs`` struct which contains ``argc`` and ``argv``
members. As is convention, ``argc`` is the number of arguments in the array of strings
each representing an individual command line argument in ``argv``.

.. _sec:registrationcallbacks:

Registration Callbacks
======================

.. _sec:toplevel:

Launching Top-Level Tasks
=========================

All computation performed by Legion must be done inside of :ref:`tasks <chap:tasks>`.
A Legion client that has :ref:`started the runtime in background mode <sec:startup>`
can launch *top-level tasks* to start new computations to be performed by Legion. 
As we will discuss :ref:`later <chap:tasks>`, each top-level task serves as the
root of a tree of tasks and operations that constitute a Legion program.

.. note::
  The Legion programming model ensures that each Legion program, consisting of a 
  tree of tasks rooted by a unique top-level task, is an isolated Legion program and 
  there are no sanctioned mechanisms by which they can cooperate, coordinate, or synchronize.
  The one exception is that the same mapper can be used to map tasks from different 
  top-level task trees and coordinate their resource utilization since they are 
  competing to run on the same hardware.

There are two mechanisms by which Legion clients can launch new top-level tasks.
First, they can use the following method to asynchronously dispatch new top-level
tasks.

.. code-block:: c++

  Future Runtime::launch_top_level_task(const TaskLauncher&);

This method will create new task top-level task as described by the ``TaskLauncher`` 
object, which has identical semantics to when it is used to 
:ref:`create subtasks <sec:subtasks>`. This method will return immediately and produce
a ``Future`` object, which we also we cover :ref:`later <sec:futures>`. Legion clients
can invoke this method an arbitrary number of times until ``wait_for_shutdown`` is
invoked. Invoking this method after ``wait_for_shutdown`` has been called will result
in undefined behavior.

In addition to creating explicit top-level tasks running inside of Legion, applications
can also choose to draft their existing threads into service as *implicit top-level tasks*.
An implicit top-level task is simply top-level task that is being executed by a thread 
not created or managed by the Legion runtime. This idiom is commonly used by applications
that are using Legion as a client accelerator runtime, where the main application is 
running along and occassionally wanting to offload computational work to Legion. Implicit
top-level tasks are started by invoking the following method.

.. code-block:: c++

  Context Runtime::begin_implicit_task(TaskID top_task_id,
                                       MapperID mapper_id,
                                       Processor::Kind proc_kind,
                                       const char *task_name = NULL,
                                       bool control_replicable = false,
                                       unsigned shard_per_address_space = 1,
                                       int shard_id = -1);

This method will draft the calling thread into service as a new top-level task to
execute the task associated with ``top_task_id`` and using ``mapper_id`` for handling
any :ref:`mapping decisions <chap:mapping>` associated with this task. The ``proc_kind``
argument specifies the kind of processor this thread should be regarded as, such as a
CPU or a GPU processor. Callers can provide an optional name for this task with the
``task_name`` parameter. In cases where this method is being invoked to start a new
:ref:`control-replicated <chap:ctrlrepl>` top-level task, the ``control_replicable``
parameter must be set to ``true``. If the implicit top-level task is being 
control-replicated, then it must also specify the number of expected invocations of
this method per process with the ``shard_per_address_space`` parameter, with the
understanding that each caller thread will be a unique :ref:`shard <sec:sharding>` 
in the execution of the control-replicated task. The ``shard_id`` parameter must be
a unique integer for each caller into ``begin_implicit_task`` for each new top-level
task being created (``shard_id`` does not need to be unique across different invocations
of ``begin_implicit_task``). Note that in the case of control-replicated implicit
top-level tasks, the runtime is performing an implicit parallel-rendezvous operation
and therefore it is incumbent upon the caller to ensure a happens-before relationship
across external (non-Legion) threads and processes when invoking ``begin_implicit_tasks`` 
for starting multiple different implicit top-level tasks. Only one implicit top-level
task can be bound to each external thread at a time. Invoking ``begin_implicit_task``
on an external thread that already has another top-level task bound to it will result
in undefined behavior.

The ``begin_implicit_task`` method returns a ``Context`` handle which
represents the enclosing :ref:`task context <sec:contexts>` and can be used to perform
runtime calls that can only occur inside of tasks. The context will remain live and
and the thread will continue to be bound to the implicit top-level task, until the 
application invokes the following method to indicate the completion of the top-level task.

.. code-block:: c++

  void Runtime::finish_implicit_task(Context ctx);

This method will unbind the executing thread from the implicit task represented
by ``ctx``. This method must be invoked on the same thread the called
``begin_implicit_task``. Invoking it on a different thread will result in 
undefined behavior. Similarly, it should be invoked exactly once for each 
implicit top-level task as multiple invocations will result in undefined behavior.
Undefined behavior will also result if this method is invoked on a ``ctx``
that was not created by a call to ``begin_implicit_task``. Note that this method
will not block to wait for the top-level task to finish. The ``Future`` object
returned by ``begin_implicit_task`` should be consulted to determine when the
task and all its child operations are actually complete (see the section on
:ref:`futures <sec:futures>` for more detail on how to use ``Future`` objects).
As soon as ``finish_implicit_task`` returns, the external thread becomes eligible 
for being bound to another implicit top-level task.

.. _sec:commandline:

Command Line Options
====================

.. rubric:: Footnotes

.. [#f1] Note that even though Legion assumes an SPMD execution style prior to start-up of the runtime, Legion is NOT an SPMD programming model; quite to the contrary! SPMD is employed only as an execution model in order to bootstrap the Legion runtime.
