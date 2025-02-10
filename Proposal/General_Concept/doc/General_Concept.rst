.. Process Model Documentation

Abstract:
This is a proposal for a code based testing framework which covers the basic tests for a ISO 26262 based test run.
The System shall be open so that parts of it can be exchanged, depending on the project/creator needs.
The system shall be generic so that, in case it is required, cross checks can be executed e.g. same functionality implemented in two different programming languages.
The system shall be based on code initially (and basically) with option to include requirements and architectural aspects later. (small actors or contributors may skip the higher level parts, of desired, with limitations to their completeness and safety aspect).
  
  
=============================

.. req:: R_001
   :title: Genreal approach
   :status: open
   :tags: general
   :author: Daniel Klotz

   The test framework shall be dynamic and adaptive to multiple environments, scopes and toolchains.

.. req:: R_002
   :title: Genreal approach
   :status: open
   :tags: general
   :author: Daniel Klotz

   The testing system shall create its test cases, test vectors, pass/fail criteria based on code and requirments including arcitecutural constraints.

.. req:: R_003
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The scope of testing and the device under test (DUT) will be determined by the assembly tool (e.g. bazel, make etc.).

.. req:: R_004
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  During test creation and setup the generator shall create documentation of its correct behaviour (Tool qualification).

.. req:: R_005
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  First base for test test case, test vector etc. creation shall be the code.

.. req:: R_006
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The test tool shall be able to support various programming languages (e.g. C, C++, Rust etc.).

.. req:: R_007
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The test tool shall be able to support various build tools (e.g. bazel, make etc.).  

.. req:: R_008
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The test tool shall automatically create test cases of what is possible with existing code, according to ISO 26262 suggestions.  

.. req:: R_008
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The test tool shall automatically create test cases for:
  - Correct function behaviour
  - Call of each function parameter within its limits (which would be the data type if no additional information is available)
  - Branch coverage, statement coverage, MC/DC coverage (implement all, but only one is required)
  - Prepare test case templates for euqivalence classes
  - Data flow analysis
  - Fault injection (Here a mechanism for generating "experienced or error guessing" shall be created)

.. req:: R_009
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  Static code analysis shall be done with external tools which will be incorporated into the generated test setup.  

.. req:: R_010
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  Main focus of the test generator will be the inductive (code based) approach as a minimum. For full compliance the deductive aspect has to be used as well (requirments, architecture etc.).#

.. req:: R_011
  :title: Genreal approach
  :status: open
  :tags: general
  :author: Daniel Klotz

  The test generator shall run completly independend so it can be used in a continous integration/testing environment.
  
This document describes the process model using PlantUML.

.. uml::

  @startuml

  title Process Model
  
  skinparam packageStyle Rectangle
  skinparam linetype ortho
  
  rectangle "Code" as Code
  rectangle "Test Implementation" as TestImpl
  rectangle "Review" as Review
  rectangle "Architecture" as Arch
  
  package "Sphinx Needs" {
      rectangle "Impl (Sphinx Needs Generated)" as Impl
      rectangle "Requirement" as Req
      rectangle "Test Case" as TestCase
      rectangle "Test Run" as TestRun
  }
  
  Code -[hidden]-> Impl
  Impl -[hidden]-> Req
  Req -[hidden]-> TestCase
  TestCase -[hidden]-> TestRun
  TestRun -[hidden]-> TestImpl
  TestImpl -[hidden]-> Review
  Review -[hidden]-> Arch
  
  Code -down-> Impl : generates
  Impl -down-> Req : contributes to
  Impl -down-> TestCase : defines
  Req -down-> TestCase : derives
  TestCase -down-> Arch : links to
  Impl -down-> TestImpl : informs
  TestCase -down-> TestImpl : defines
  TestImpl -down-> TestRun : executed in
  Review -down-> Req : refines
  
  left to right direction
  
  @enduml
