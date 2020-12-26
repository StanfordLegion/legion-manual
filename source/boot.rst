
.. _chap:boot:

Bootstrapping
*************

Regardless of your choice of language to use for writing Legion
programs, all Legion programs must first begin by bootstrapping
the runtime. Legion is designed to work in both single-process and
multi-process environments, with multi-process being the much more
common deployment methodology. Legion is almost entirely ambivalent 
about how these processes are created. In single-process execution,
the program can be started as any other. For multi-process execution,
we recommend using `mpirun <https://www.open-mpi.org/doc/v4.0/man1/mpirun.1.php>`_
as a highly scalable and portable way to create these processes. You
are also welcome to use any of the GASNet supported spawners. For
the most recent documantation on these spawners please visit the 
`GASNet website <https://gasnet.lbl.gov/>`_. The only constraints
that Legion requires for multi-process execution, are the ones
enforced upon it by GASNet (e.g. the presence of a 
`PMI implementation <https://wiki.mpich.org/mpich/index.php/PMI_v2_API>`_). 

Once processes are running, you need to startup the Legion runtime
inside of these processes. We assume that prior to starting the Legion
runtime, these processes are operating in an 
`SPMD execution style <https://en.wikipedia.org/wiki/SPMD>`_ where
each process will be making the same sequence of API calls to configure
the Legion runtime prior to the call to start the runtime [#f1]_. If you 
violate this basic principle, then undefined behavior will result. We
also assume that there will be exactly one thread bootstrapping the 
Legion runtime in each process. If you violate this principle, undefined
behavior will result. Often this thread is the same thread that calls
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
Legion programs. There are two optional runtime configuration calls
that can be performed before start-up of the Legion runtime. First,
is the static method to set the task ID of the top-level task to be 
dispatched by the runtime upon the completion of start-up.

.. code-block:: c++

  static void Legion::Runtime::set_top_level_task_id(TaskID);

This method can be called repeatedly, but the value set by the the 
last invocation will the one task actually dispatched. Note that this
is more of a legacy (but not deprecated) method as there are
:ref:`other ways to dispatch top-level tasks after starting the runtime <sec:toplevel>`.
In addition to configuring the top-level task to run, you can also 
specify the ID of the mapper to use for mapping the top-level task.

.. code-block:: c++

  static void Legion::Runtime::set_top_level_task_mapper_id(MapperID);

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

  typedef void (*RegistrationCallbackFnptr)(Legion::Machine, Legion::Runtime*, const std::vector<Legion::Processor>&);  

Function pointers are registered with the Legion runtime using this method:

.. code-block:: c++
  
  static void Legion::Runtime::add_registration_callback(RegistrationCallbackFnptr);

After the runtime has been fully initialized, but before any top-level
tasks have run, then the runtime will proceed to invoke each of these
callbacks in the order in which they were added (FIFO). We discuss 
:ref:`the semantics of these callbacks <sec:registrationcallbacks>` later.
A common idiom for Legion libraries is to record a registration callback, 
and then perform all their registrations once the runtime has started and
has more facilities to offer.

The second class of preregistration functions are methods that are
used for registering resources such as task variants, reduction 
operations, etc. All of these resources can also be registered later after
the runtime is started, but for applications that know these resources
up-front, it can be useful to preregister these resources. 


.. _sec:startup:

Starting the Legion Runtime
===========================

.. _sec:registrationcallbacks:

Registration Callbacks
======================

.. _sec:toplevel:

Launching Top-Level Tasks
=========================

.. _sec:commandline:

Command Line Options
====================

.. rubric:: Footnotes

.. [#f1] Note that even though Legion assumes an SPMD execution style prior to start-up of the runtime, Legion is NOT an SPMD programming model; quite to the contrary! SPMD is employed only as an execution model in order to bootstrap the Legion runtime.
