.. Legion Manual documentation master file, created by
   sphinx-quickstart on Fri Nov 27 01:30:13 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the Legion Manual
############################

The `first paper <https://legion.stanford.edu/pdfs/sc2012.pdf>`_ 
describing the Legion programming model was published in 2012.
Since then, there has been enormous progress and many people 
have contributed to the project. Throughout this period new application 
developers have learned Legion through a combination of examples, lore 
from other members of the project, research papers and reading the 
source code of the Legion implementation.  The intention here is to put 
down, in a systematic fashion, a complete specification of the Legion
API and its semantics for users to reference when developing Legion programs.

.. _sec:reading:

Reading This Manual
===================

The Legion API is quite large and a comprehensive knowledge of it is *not*
required to write useful programs. Therefore, we've written this manual in
a sytle that is not conduicive to being read front-to-back. Instead the manual
should be used as a reference for users to access when they need to understand
a specific feature of the Legion runtime API. We've taken great care to
add copious hyperlink referenes to related sections and features throughout the 
text of the manual, thereby facilitating easy pointer-chasing of related
features. We encourage readers to employ a depth-first-search algorithm when
reading the manual: continue clicking on hyperlinks, following them down the
graph of dependences until you reach something that you already know or
you read something with no further links to follow; only then retreat back 
along the path and continue reading. The more times you refer to the manual,
the shallower your traversals will become as you accumulate knowledge and the
quicker you will come to understand individual features.

.. toctree::
   :maxdepth: 2
   :numbered:
   :caption: Contents:

   start
   philosophy
   boot
   tasks
   regions
   privileges
   coherence
   operations
   partitioning
   indexops
   mapping
   tracing
   replication
   noncanonical
   interop
   tools



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
