
.. _chap:philosophy:

The Philosophy of Legion
************************

While reading section is not strictly required for understanding
any of the features in the Legion runtime API, there are a number of
design decisions and aspects of Legion that will make more sense if
users have some degree of understand of the philosophy of Legion.
For the best understanding of the philosphy of Legion, we encourage
you to read the `many peer-reviewed publications on Legion 
<https://legion.stanford.edu/publications/>`_ that articulate in
great detail the design of Legion. The rest of this section will
briefly introduce the general themes of the philosophy of Legion
at a degree of details sufficient for providing context for the
features described in this manual.

.. _sec:costmetrics:

Design Cost Metric
==================

When it comes to designing an API, there are numerous trade-offs
that can be made. One over-arching cost metric though guides many
of the trade-offs in Legion. The Legion API always prefers to optimize
for the long-term costs of using and maintaining software over
the course of many years, even at the expense of short-term costs 
for writing a program or adding a new feature. The rationale behind this
logic is simple: if a program is going to be around for many years
and maintained by large groups of people (as most HPC-style programs are),
then the additional cost of a few extra minutes or hours of one person's
time writing some code, can be easily amortized by saving many hours
or days of time for the people that come later and have to use, maintain,
and change that code. Many developers initially find the extra costs
to getting something working in Legion off-putting. In the long run 
though, more time is saved this way and the resulting software is easier
to use, maintain, and modify. We encourage all of our users to consider
this metric carefully and think about those who will come after them
when choosing to write Legion code. In many ways, the Legion API will
literally force developers to write better long-term code.

.. _sec:mechfrompolicy:

Seperating Mechanism from Policy
================================

Part of the core design philosophy for Legion is that there should be
a strict separation between the *mechanism* of describing a computation
to be performed, and the *policy* descisions that determine how that
computation should be executed. The Legion API is actually two completely
independent APIs: an *application API* that is used to describe the computation
being performed, and a *mapping API* that is used to describe how to map
computations onto a target machine. The application API forces users to 
develop *machine-independent* programs that do not depend on any specific
underlying details of the hardware. The computation is expressed instead in
an abstract and parametrized way that provides many degrees of freedom for
determining how that computation is ultimately mapped. To actually execute
Legion applications on real machines, users must develop *mappers* that
implement :ref:`the Legion mapping API <chap:mapping>`. Mappers make decisions
about how to map the machine-independent application program onto a 
target machine. Importantly, decisions made mappers using the mapping
API never influence the correctness of the program being run, only the
performance. Therefore, Legion enforces a strict separation of mechanism in
the form of a machine-independent computation, from policy in the form
of mappers that make decisions about how the computation is executed.

The process of disentangling the machine-independent program from mapping
decisions is unintuitive for many users because they have spent their entire
programming lives writing code where these two aspects are conflated in
one way or another. They often stuggle initially to think in a form that
is sufficiently abstract about their computation and all the different 
parameters of potential mapping they should abstract when developing the
machine-independent part of the code. The process of doing this however
is precisely the exercise that ensures that Legion programs are highly
portable to new machines, including machines that were not even conceived
of when the program was originally written. There is a noticeable quality
to well abstracted Legion programs that ensures that are highly portable:
such programs are almost never revised after their initial development,
except for the development of new mappers for them. Users that find themselves
consistently needing to revise their application-side program should 
introspect their program carefully to look for ways that they have not
fully abstracted all the machine details from the application.

This aspect of Legion is one of the hardest parts for new beginners to 
understand, but it also one of the most important. We encourage new users
to look at `good examples <https://github.com/StanfordLegion/Legion-SNAP>`_
of well-decoupled Legion programs.

.. _sec:deferredexecution:

Deferred Execution
==================

.. _sec:prodvsctrl:

Productivity versus Expressivity and Control
============================================

.. _sec:funcimperative:

Coarse-Grained Functional/Fine-Grained Imperative
=================================================

.. _sec:composition:

Radical Software Composition
============================



