
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
in these repositories; you assume all risks when using them.

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
should work correctly [#f1]_.

Legion has been successfully compiled and run on both Linux and Darwin (Mac OS) 
operating systems. There is currently no support for Windows.

For multi-node execution, we require a recent installation of 
`GASNet <https://gasnet.lbl.gov/>`_. We provide `custom-configured versions 
of GASNet optimized for Legion <https://github.com/StanfordLegion/gasnet/>`_.
You are encouraged to use them, or to build and install GASNet from scratch with 
your own custom settings for GASNet's configuration. Be aware that GASNet has 
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
you encounter a recent version that does not work please :ref:`create an issue <sec:help>`.

.. _sec:build:

Buliding From Source
====================

For the convenience of both ourselves, and our users, we maintain two different 
build systems for Legion based on `Make <https://www.gnu.org/software/make/>`_
and `CMake <https://cmake.org/>`_. The Make build system is lower-level, 
more transparent, and requires more input from the user. The CMake build system 
is higher-level, easier to use, and harder to fix when things go wrong. You 
can pick which build system you are most comfortable using. We do not recommend 
mixing build systems.

.. _subsec:configuration:

Configuration Options
---------------------

Both the Make and CMake build systems provide a number of build-time 
configuration options that you might be interested in controlling. We 
enumerate these here for completeness. These configurations are documented 
by defined macros in the generated ``legion_defines.h`` header file which 
will be included through the ``legion.h`` header file so they can leveraged 
in application code if desired.

* ``PRIVILEGE_CHECKS`` - Enable dynamic privilege checks on Legion accessors. 
  Will verify that each read/write/reduce performed to a region in a task 
  abides by the declared privileges. These checks are disabled by default.
* ``BOUNDS_CHECKS`` - Enable dynamic bounds checks on Legion accessors. Will 
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
* ``LEGION_USE_CUDA`` - Enable support for NVIDIA GPUs which using Legion. This 
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
  and you may be asked to use this setting to dump log files if you report a bug.
* ``LEGION_USE_LIBDL`` - Enable use of LibDL for dynamic lookup of symbols from 
  function pointers for global registration. This is enabled by default.

There are additional build-time configuration properties that you can change in the 
`configuration header file <https://gitlab.com/StanfordLegion/legion/-/blob/master/runtime/legion/legion_config.h>`_. 
The default values for these settings will be acceptable for the vast majority 
of Legion applications and you should not need to the alter them in most cases.

.. _subsec:makebuild:

Make Build System
-----------------

.. _subsec:cmakebuild:

CMake Build System
------------------

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
Many users find it convenient to import the Legion namespace
into their programs, although this is optional.

.. code-block:: c++

  using namespace Legion;

The rest of this manual will document the user-facing types and
functions of the Legion runtime in C++. For completeness, we will
briefly detail bindings of the Legion runtime in other languages
for users that are interested.

.. _subsec:regent:

Regent
------

The `Regent programming language <http://regent-lang.org/>`_ is a programming
language designed explicitly for the Legion programming model. It automates many
of the more sophisticated parts of writing code in Legion, and provides `advanced
type checking, safety features <https://legion.stanford.edu/pdfs/regent2015.pdf>`_,
and `performance <https://legion.stanford.edu/pdfs/cr2017.pdf>`_ 
`optimizations <https://legion.stanford.edu/pdfs/parallelizer2019.pdf>`_ for the 
Legion programming model. If your goal is simply to play around with Legion to get a
better feel for the programming model, we *strongly* encourage you to use Regent
over the C++ interface as you will be considerably more productive. The C++
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
that wrap pointers to the underlying C++ primitives. It is your
responsibility to ensure that these handles are explicitly deleted
in order to avoid memory leaks. The C bindings are compatible with
C99 and should also enable you to embed Legion into any language
that supports as C99 foreign function interface.

.. _subsec:python:

Python Bindings
---------------

Legion provides a `legion_cffi.py <https://gitlab.com/StanfordLegion/legion/-/blob/master/bindings/python/legion_cffi.py.in>`_
that will import the :ref:`C bindings <subsec:langc>` into Python through the Python
`CFFI interface <https://cffi.readthedocs.io/en/latest/>`_. You can then
invoke the Legion runtime directly from Python. In addition to these
bindings, you can also build ``legion_python``, a (near) drop-in replacement
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
are currently present to write interesting Fortran programs. You can find
all of the `tutorial programs <https://legion.stanford.edu/tutorial/>`_
written in Fortran `here <https://gitlab.com/StanfordLegion/legion/-/tree/master/tutorial/fortran>`_.

.. _sec:help:

Getting Help
============
If you encounter problems using Legion, there are two primary ways to get help. 
First, if you have a general question or problem, the best way to get help is to 
email the `Legion Users' mailing list <mailto:legionusers@googlegroups.com>`_. 
This will broadcast your question or problem to the Legion developers as well as 
other Legion users who may also be able to answer your question sooner. You can also 
search `previous email threads <https://groups.google.com/g/legionusers>`_ on 
this mailing list to see if anyone else has encountered similar issues to your own.

If you believe that you have actually found a bug or need a feature implemented 
then please create an issue on our 
`github issue tracker <https://github.com/StanfordLegion/legion/issues>`_.
We will triage your issue, assign someone to work on it, and apply flags 
indicating when it will likely be resolved. Please make a significant effort to 
reduce any reproducing programs down to the smallest possible size. The time 
required for us to respond to and fix your bug will be somewhere between
super-linear and exponential in the size of the program you ask us to investigate.

.. rubric:: Footnotes

.. [#f1] We are still waiting for the new Arm Helium vector instructions to become more widespread so we can use them rather than implementing the now deprecated Neon instructions
.. [#f2] While ``legion_python`` is a replacement interpreter, it still relies on an unmodififed CPython interpreter for execution of Python code so we have full compatiblity with existing Python. The only difference is that we are bootstrapping the Python interpreter inside of Legion and Realm. 
