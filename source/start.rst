
.. _chap:start:

Getting Started
***************

The `Legion homepage <legion.stanford.edu>`_ contains
links to everything associated with the project, including a set of
`tutorials <https://legion.stanford.edu/tutorial/>`_ that are distinct
from this manual, `instruction videos <https://legion.stanford.edu/bootcamp2017/>`_
of past Legion bootcamps for new users, 
`doxygen documentation <https://legion.stanford.edu/doxygen/>`_, and 
`academic papers <https://legion.stanford.edu/publications/>`_ explaining the 
architecture of Legion.  The Legion source code can be downloaded either
from our primary repository at `gitlab <https://gitlab.com/StanfordLegion/legion>`_ or
our mirror on `github <https://github.com/StanfordLegion/legion/>`_. Both repositories 
contain a ``stable`` branch that represents the most recent release of the
runtime (usually released every three months). The ``master`` branch is our primary
development branch and should be considered our "nightly" build of Legion. It is
generally kept in good working order, but can undergo breaks during the course of 
development which will be quickly remedied. There will be other development branches
in these repositories; users assume all risks when using them.

.. _sec:requirements:

Requirements
============

There are a number of language requirements that we assume for building Legion:

* C++ - A c++11 compliant compiler is required. Legion is tested with multiple 
  versions of gcc, clang, nvc++ (formerly pgc++). The icc compiler has also been 
  successfully used in the past.
* C - Legion's C bindings require a c99 compiant compiler. Legion is tested with 
  many versions of gcc and clang.
* Fortran - Legion's Python bindings require an f90 compliant compiler. Legion 
  is tested with gfortran.
* Python - Legion's Python bindings require an installtion of Python 3.5 or higher. 
  Legion is tested with CPython.
* CUDA - Legion's CUDA bindings require CUDA 8 or later. Legion is tested with 
  many versions of nvcc.

Legion has been successfully compiled and run on machines with x86, x86\_64, 
PowerPC, and Arm instruction sets. Arm support is not fully optimized, but 
should work correctly [#f1]_. Legion has been successfully compiled and run 
on both Linux and Darwin (Mac OS) operating systems. 

.. note::
  There is currently no support for Windows.

For multi-node execution, we require a recent installation of 
`GASNet <https://gasnet.lbl.gov/>`_. We provide `custom-configured versions 
of GASNet optimized for Legion <https://github.com/StanfordLegion/gasnet/>`_.
Users are encouraged to use them, or to build and install GASNet from scratch with 
their own custom settings for GASNet's configuration. Be aware that GASNet has 
numerous `options <https://gasnet.lbl.gov/dist/README>`_ that can greatly 
impact performance. Please run extensive performance tests to ensure that any
custom installations are achieving the best possible GASNet performance. 
Do not under any circumstances use a GASNet installation that has been 
pre-installed by an adminstrator into the module system; it is highly likely 
to have been tuned for UPC programs with many tiny RDMA reads and writes and 
is therefore configured in exactly the wrong way for Legion usage.

There are many optional peripheral dependences in Legion, such as for 
`ZLib <https://zlib.net/>`_ and `HDF5 <https://www.hdfgroup.org/solutions/hdf5/>`_. 
In general, Legion should work with recent versions of these libraries, but if 
users encounter a recent version that does not work please :ref:`create an issue <sec:help>`.

.. _sec:build:

Buliding From Source
====================

For the convenience of both ourselves, and our users, we maintain two different 
build systems for Legion based on `Make <https://www.gnu.org/software/make/>`_
and `CMake <https://cmake.org/>`_. The Make build system is lower-level, 
more transparent, and requires more input from the user. The CMake build system 
is higher-level, easier to use, and harder to fix when things go wrong. Users 
can select which build system they are most comfortable using. We do not recommend 
mixing build systems.

.. _subsec:configuration:

Configuration Options
---------------------

Both the Make and CMake build systems provide a number of build-time 
configuration options that users might be interested in controlling. We 
enumerate these here for completeness. These configurations are documented 
by defined macros in the generated ``legion_defines.h`` header file which 
will be included through the ``legion.h`` header file so they can leveraged 
in application code if desired.

* ``LEGION_PRIVILEGE_CHECKS`` - Enable dynamic privilege checks on Legion 
  accessors. Will verify that each read/write/reduce performed to a region in a 
  task abides by the declared privileges. These checks are disabled by default.
* ``LEGION_BOUNDS_CHECKS`` - Enable dynamic bounds checks on Legion accessors. Will 
  verify that each access to a region inside a task falls within in the bounds 
  of the declared region. Warning: this can be very expensive as it adds a branch 
  literally every read and write instruction touching a region. These checks are
  disabled by default.
* ``LEGION_MAX_DIM`` - Specify the maximum number of dimensions that Legion can 
  support for index spaces. The default value is ``3``, but values between ``1`` 
  and ``9`` (inclusive) are allowed. Note that compilation time will likely 
  scale quadratically in the number of dimensions.
* ``LEGION_MAX_FIELDS`` - Specify the maximum number of fields that can be 
  dynamically allocated in a single field space. This number must be a power 
  of ``2``. The default is ``256``. The minimum allowed is ``64``. There is no 
  maximum, but runtime overhead will scale proportionally to this value so it is
  advantageous to ensure that the bound is tight.
* ``LEGION_USE_CUDA`` - Enable support for NVIDIA GPUs with Legion. This 
  is off by default.
* ``LEGION_USE_ZLIB`` - Enable support for using ZLib for dumping compressed 
  log files such as profiling logs. This is enabled by default.
* ``LEGION_REDOP_COMPLEX`` - Enable support for bulit-in reduction operators 
  using `complex numbers <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/mathtypes/complex.h>`_. This is disabled by default.
* ``LEGION_REDOP_HALF`` - Enable support for built-in `half-precision floating 
  point <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/mathtypes/half.h>`_ reduction operators. This is disabled by default.
* ``LEGION_WARNINGS_FATAL`` - Promote warnings to fatal errors. This is disabled 
  by default.
* ``LEGION_SPY`` - Enabled detailed :ref:`Legion Spy <sec:spy>`
  logging for Legion. This is usefull for validating the correctness of the runtime 
  and users may be asked to use this setting to dump log files if they report a bug.
* ``LEGION_USE_LIBDL`` - Enable use of LibDL for dynamic lookup of symbols from 
  function pointers for global registration. This is enabled by default.

Both the :ref:`make <subsec:makebuild>` and :ref:`cmake <subsec:cmakebuild>` systems
have different ways of specifying these configuration parameters, which we will describe 
shortly. There are additional build-time configuration properties that users can change in the 
`configuration header file <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/legion/legion_config.h>`_. 
The default values for these settings will be acceptable for the vast majority 
of Legion applications and users should not need to alter them in most cases.

.. _subsec:makebuild:

Make Build System
-----------------

When using the Make build system, users can either develop their own Makefile
or use our `template Makefile <https://gitlab.com/StanfordLegion/legion/-/blob/master/apps/Makefile.template>`_.
The Legion Make build system requires that all users set the ``LG_RT_DIR`` in their
environment. The ``LG_RT_DIR`` environment variable should point to location of
the ``runtime`` 
directory containing the `Legion source code <https://gitlab.com/StanfordLegion/legion/-/tree/master/runtime>`_. 
For example, for a user ``joe`` who has place the Legion repository in his home 
directory, then ``LG_RT_DIR`` would point to ``/home/joe/legion/runtime``. 

.. note::
  If users want to just build and install the Legion header files and libraries, simply
  copy the template Makefile into a temporary directory and invoke ``make``. If 
  static libraries (``.a``) are desired, then set ``SHARED_OBJECTS=0`` in the Makefile;
  alternativey, for shared objects (``.so`` or ``.dynlib`` depending on the operating system), 
  then set ``SHARED_OBJECTS=1`` in the Makefile. After the build completes, set 
  ``PREFIX`` to th etarget install directory and invoke ``make install``.

There are a number of variables that users can set to control the configuration of the 
Legion runtime at build-time.

* ``DEBUG`` - If set to ``1`` this will build the runtime in debug mode with optimizations
  off and extra debugging checks enabled. Setting to ``0`` will turn on optimizations and
  turn off many checks.
* ``MAX_DIM`` - Set the maximum number of dimensions that Legion will support in an 
  :ref:`index space <sec:indexspaces>`. This will configure the ``LEGION_MAX_DIM`` variable.
* ``MAX_FIELDS`` - Set the maximum number of fields that Legion will support in a 
  :ref:`field space <sec:fieldspaces>`.
* ``OUTPUT_LEVEL`` - Set the compile-time minimum output logging level. All logging statements
  with a level lower than this will be statically optimized away. There are eight different
  levels listed in ascending order: ``LEVEL_SPEW``, ``LEVEL_DEBUG``, ``LEVEL_INFO``, 
  ``LEVEL_PRINT``, ``LEVEL_WARNING``, ``LEVEL_ERROR``, ``LEVEL_FATAL``, and ``LEVEL_NONE``.
* ``USE_FORTRAN`` - Enable support for Legion's fortran bindings.
* ``USE_CUDA`` - Enable support for using `CUDA <https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html>`_ 
  inside of Legion tasks. This will also configure the ``LEGION_USE_CUDA`` definition.
* ``USE_OPENMP`` - Enable support for using `OpenMP <https://www.openmp.org/>`_ inside of Legion tasks.
* ``USE_GASNET`` - Enable multi-node Legion execution with `GASNET <https://gasnet.lbl.gov/>`_.
* ``USE_ZLIB`` - Enable support for dumping compressed logfiles with `zlib <https://zlib.net/>`_. 
  This will configure ``LEGION_USE_ZLIB`` setting. 
* ``USE_LIBDL`` - Enable support for doing reverse function pointer name lookup for portable function
  pointers using `LibDL <https://docs.oracle.com/cd/E86824_01/html/E54772/libdl-3lib.html>`_. 
  This will configure ``LEGION_USE_LIBDL`` setting.
* ``USE_LLVM`` - Enable support for specifying tasks from `LLVM IR <https://llvm.org/>`_.
* ``USE_HDF`` - Enable support for `HDF5 files <https://www.hdfgroup.org/solutions/hdf5/>`_.
* ``USE_SPY`` - Enable support for detailed :ref:`Legion Spy <sec:spy>` logging.
  This will configure the ``LEGION_SPY`` setting.
* ``USE_HALF`` - Enable support for `half precision types <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/mathtypes/half.h>`_ in Legion's :ref:`built-in reduction operators <subsec:builtinredops>`.
* ``USE_COMPLEX`` - Enable support for `complex types <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/mathtypes/complex.h>`_ in Legion's :ref:`built-in reduction operators <subsec:builtinredops>`.
* ``SHARED_OBJECTS`` - Indicate whether the Legion and Realm libraries should be built as shared objects (e.g. ``.so`` or ``.dylib``) or as static libraries (``.a``). When building an application, then this will also specify how these objects are linked to the application.
* ``BOUNDS_CHECKS`` - Enable dynamic bounds checks on every single access to a region through an :ref:`accessor <sec:accessors>`. This will also configure the ``LEGION_BOUNDS_CHECKS`` setting. For most applications this is *very* expensive and users could observe significant slowdowns.
* ``PRIVILEGE_CHECKS`` - Enable dynamic privilege checks on every single access to a region through an :ref:`accessor <sec:accessors>`. This will also configure the ``LEGION_PRIVILEGE_CHECKS`` setting. This will be significantly less expensive than bounds checks in most cases.
* ``MARCH`` - Specify the target CPU `machine architecture <https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html>`_.
* ``GPU_ARCH`` - Specify the target GPU architecture. Can be one of ``kepler``, ``k20``, ``k80``, ``maxwell``, ``pascal``, ``volta``, ``turing``, or ``ampere``.
* ``CONDUIT`` - Specify the target GASNet architecture. Can be one of ``ibv``, ``ucx``, ``gemini``, ``aires``, ``psm``, ``mpi``, or ``udp``. 
* ``REALM_NETWORKS`` - Specify the target Realm networking layer to use. Can be one of ``gasnet1`` (legacy GASNet), ``gasnetex`` (modern GASNet), or ``mpi`` (for sending messages directly over MPI, not recommended).
* ``GASNET`` - Specify the directory where GASNet has been installed.
* ``CUDA`` - Specify the directory where CUDA has been installed.
* ``HDF_ROOT`` - Specify the directory where HDF has been installed.
* ``PREFIX`` - Specify the directory where Legion and this application should be installed. The build system
  will place Legion files in the ``bin``, ``include``, and ``lib`` subdirectories of this prefix. 
* ``OUTFILE`` - Specify the name of the binary or shared object to produce. If left empty, it will default to ``liblegion.so`` or ``liblegion.a`` depending on the setting of ``SHARED_OBJECTS``.
* ``CC_SRC`` - List the names of all C programming language files to be compiled for the application.
* ``CXX_SRC`` - List the names of all C++ programming language files to be compiled for the application.
* ``CUDA_SRC`` - List the names of al CUDA programming language files to be compiled for the application.
* ``FORT_SRC`` - List the names of all Fortran programming language files to be compiled for the application.
* ``ASM_SRC`` - List the name of all the assembly language files to be compiled for the application.
* ``INC_FLAGS`` - List of all include directories to be passed to all compiler invocations.
* ``CC_FLAGS`` - List of flags for all C++ compiler invocations.
* ``FC_FLAGS`` - List of flags for all Fortran compiler invocations.
* ``NVCC_FLAGS`` - List of flags for all CUDA compiler invocations.
* ``SO_FLAGS`` - List of flags for linking shared libraries.
* ``LD_FLAGS`` - List of flags for linking application binaries.

In addition to the variables above, users can also leverge many of the common 
`GNU Make implicit compilation flags <https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html>`_
for specifying argument to different compilers. At the end of the Makefile, there should 
be a single line statement:

.. code-block:: make

  include $(LG_RT_DIR)/runtime.mk

The ``include`` statement will invoke the Legion runtime's
`Make-based build system <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/runtime.mk>`_
to build the application or libraries. The Makefile can be run via an invocation of ``make`` as usual.
(We recommend employing the ``-j`` flag for a parallel build.) After the build
is successful, then ``make install`` will install the application or library/libraries into
the directory specified by ``PREFIX``.

.. _subsec:cmakebuild:

CMake Build System
------------------

.. warning::
  TODO: somebody who understands the insanity of cmake needs to write this section

.. _sec:languages:

Languages
=========

The Legion runtime is written in C++ and therefore
its native API is also expressed in C++. The vast majority
of the Legion C++ interface is contained in the 
`legion.h <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/legion.h>`_
header file. Including the ``legion.h`` header file into any C++ program
will import all the types and functions required for any Legion
program. No other Legion header files need be included. All 
Legion types and functions exist within the ``Legion`` namespace.
Users may find it convenient to import the Legion namespace
into their programs, although this is optional.

.. code-block:: c++

  using namespace Legion;

Note that while Legion is written in C++, the Legion programming model
is at odds with many of the core concepts in the C++ 
`execution model <https://corecppil.github.io/CoreCpp2019/Presentations/Bryce_C++_Execution_Model.pdf>`_
and `memory model <https://en.cppreference.com/w/cpp/language/memory_model>`_.
The Legion programming model imposes much stricter requirements on memory
accesses than the C++ memory model does, making many aspects of C++'s
memory model illegal in Legion programms. Legion also imposes strict
requirements on execution, disallowing many C++ features such as thread
creation or the use of ``std::async``. However, Legion will also relax
some aspects of C++, such as the requirement that each object live in 
exactly one location in memory. We will detail all of these restrictions 
and relaxations throughout this manual.

The rest of this manual will document the user-facing types and
functions of the Legion runtime in C++. For completeness, we will
briefly detail bindings of the Legion runtime in other languages
if users are interested.

.. _subsec:regent:

Regent
------

The `Regent programming language <http://regent-lang.org/>`_ is a programming
language designed explicitly for the Legion programming model. It automates many
of the more sophisticated parts of writing code in Legion, and provides `advanced
type checking, safety features <https://legion.stanford.edu/pdfs/regent2015.pdf>`_,
and `performance <https://legion.stanford.edu/pdfs/cr2017.pdf>`_ 
`optimizations <https://legion.stanford.edu/pdfs/parallelizer2019.pdf>`_ for the 
Legion programming model. If the goal is simply to play around with Legion to get a
better feel for the programming model, we *strongly* encourage users to consider Regent
over the C++ interface as they will be considerably more productive. The C++
interface is more for expert users that want very detailed control over what the
runtime is doing and are therefore likely to build their own producitivity 
abstraction over top of Legion.

.. _subsec:langc:

C Bindings
----------

The Legion runtime comes equipped with C bindings for targetting
the C programming language. These bindings can be found in the 
`legion_c.h <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/legion/legion_c.h>`_
header file. Since there are no namespaces in C, all legion types
and functions are prefixed by ``legion_`` to avoid collisions with
other names. The Legion C API works by creating explicit handles
that wrap pointers to the underlying C++ primitives. It is a user's
responsibility to ensure that these handles are explicitly deleted
in order to avoid memory leaks. The C bindings are compatible with
C99 and should also enable users to embed Legion into any language
that supports a C99-compatible foreign function interface.

.. _subsec:python:

Python Bindings
---------------

Legion provides a `legion_cffi.py <https://gitlab.com/StanfordLegion/legion/-/blob/master/bindings/python/legion_cffi.py.in>`_
that will import the :ref:`C bindings <subsec:langc>` into Python through the Python
`CFFI interface <https://cffi.readthedocs.io/en/latest/>`_. Users can then
invoke the Legion runtime directly from Python. In addition to these
bindings, users can also build ``legion_python``, a (near) drop-in replacement
Python interpreter that will execute any Python program inside of the top-level
task of a Legion program [#f2]_. Normal Python programs will execute like normal, 
but Legion-aware Python programs can leverage the power of the Legion runtime
for distributed and parallel execution. This functionality is used by other
higher-level system such as `Pygion <https://legion.stanford.edu/pdfs/pygion2019.pdf>`_
which embeds the Legion programming model more completely into Python than
simply importing the C bindings. The Pygion bindings can be found in the
`pygion module <https://gitlab.com/StanfordLegion/legion/-/blob/master/bindings/python/pygion.py>`_.

.. _subsec:fortran:

Fortran Bindings
----------------

Legion contains bindings for the Fortran programming language in
`legion_f.f90 <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/legion/legion_f.f90>`_.
The Legion Fortran bindings are currently incomplete, but enough bindings
are currently present to write interesting Fortran programs. Users can find
all of the `tutorial programs <https://legion.stanford.edu/tutorial/>`_
written in Fortran `here <https://gitlab.com/StanfordLegion/legion/-/tree/master/tutorial/fortran>`_.

.. _sec:help:

Getting Help
============
If users encounter problems using Legion, there are two primary ways to get help. 
First, for general questions or problems, the best way to get help is to 
email the `Legion Users' mailing list <mailto:legionusers@googlegroups.com>`_. 
This will broadcast questions and problems to the Legion developers, as well as 
other Legion users who may also be able to answer questions or suggest solutions
to problems much sooner. Users should also search 
`previous email threads <https://groups.google.com/g/legionusers>`_ on 
this mailing list to see if anyone else has encountered similar issues previously.

If users believe that they have actually found a bug or need a feature implemented 
then please create an issue on our 
`github issue tracker <https://github.com/StanfordLegion/legion/issues>`_.
We will triage each issue, assign someone to work on it, and apply flags 
indicating when it will likely be resolved. Please make a significant effort to 
reduce any reproducing programs down to the smallest possible size. The time 
required for us to respond to and fix an issue will be somewhere between
super-linear and exponential in the size of the program we are asked to investigate.

.. rubric:: Footnotes

.. [#f1] We are still waiting for the new Arm Helium vector instructions to become more widespread so we can use them rather than implementing the now deprecated Neon instructions
.. [#f2] While ``legion_python`` is a replacement interpreter, it still relies on an unmodififed CPython interpreter for execution of Python code so we have full compatiblity with existing Python. The only difference is that we are bootstrapping the Python interpreter inside of Legion and Realm. 
