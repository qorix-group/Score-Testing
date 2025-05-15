.. Tutorial: Using the Rust Proxy Demonstrator
   ==========================================

.. need:: Requirement
   :id: REQ_TUTORIAL
   :title: Tutorial for System Usage
   :status: open
   :tags: tutorial; usage
   :links: REQ_OVERVIEW
   :description: A user-friendly tutorial explaining how to use the Rust Proxy Demonstrator and apply it to other sources, including PlantUML diagrams.

Introduction
------------

The Rust Proxy Demonstrator is a Rust library implementing a proxy-based test system for classes (`Class1`, `Class2`, `Class3`) with comprehensive unit and integration tests. This tutorial guides you through using the system and adapting it to other sources, such as different Rust projects or data sources.

.. uml::
   :caption: System Architecture Overview

   @startuml
   package "Rust Proxy Demonstrator" {
     [Class1] --> [Class3]
     [Class2] --> [Class3]
     [TestClass1] --> [Class1]
     [TestClass2] --> [Class2]
     [TestClass3] --> [Class3]
     [InstrumentedClass1] --> [Class1]
     [InstrumentedClass2] --> [Class2]
     [InstrumentedClass3] --> [Class3]
     [TestTracker] --> [InstrumentedClass1]
     [TestTracker] --> [InstrumentedClass2]
     [TestTracker] --> [InstrumentedClass3]
   }
   [Unit Tests] --> [TestClass1]
   [Unit Tests] --> [TestClass2]
   [Unit Tests] --> [TestClass3]
   [Integration Tests] --> [TestClass1]
   [Integration Tests] --> [TestClass2]
   [Integration Tests] --> [TestClass3]
   @enduml

.. note:: To view the PlantUML diagram, copy the code between `@startuml` and `@enduml` into a PlantUML viewer like https://www.plantuml.com/plantuml/ or a VS Code PlantUML extension.

Prerequisites
-------------

.. need:: Configuration
   :id: CONFIG_PREREQ
   :title: Prerequisites for Usage
   :status: implemented
   :tags: setup; configuration
   :links: REQ_TUTORIAL
   :description: Describes the tools and environment required to run the Rust Proxy Demonstrator.

Before using the system, ensure the following are installed:

- **Rust**: Version 1.80 or higher, installed via `rustup`:
  .. code-block:: powershell

     curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
     rustc --version

- **Cargo**: Included with Rust, used for building and running tests.
- **VS Code**: Recommended for development, with the Rust-Analyzer extension.
- **Project Directory**: The project should be located at `D:\Temp\Qorix\Proxy Test\RUST\rust_proxy`.

Steps to Use the System
-----------------------

.. need:: Specification
   :id: SPEC_USAGE
   :title: Steps to Use the System
   :status: implemented
   :tags: usage; tutorial
   :links: REQ_TUTORIAL
   :description: Step-by-step instructions for using the Rust Proxy Demonstrator.

Follow these steps to use the `rust_proxy` system:

1. **Navigate to the Project Directory**:
   Ensure you are in the project directory:
   .. code-block:: powershell

      cd D:\Temp\Qorix\Proxy Test\RUST\rust_proxy

   If the project is in a Git repository, clone it:
   .. code-block:: powershell

      git clone <repository-url>
      cd rust_proxy

2. **Verify Project Structure**:
   Confirm the project structure:
   .. code-block:: text

      rust_proxy/
      ├── Cargo.toml
      ├── src/
      │   ├── lib.rs
      │   ├── main.rs
      │   ├── class1.rs
      │   ├── class2.rs
      │   ├── class3.rs
      │   ├── test_tracker.rs
      │   ├── test_class1.rs
      │   ├── test_class2.rs
      │   ├── test_class3.rs
      ├── tests/
      │   ├── integration.rs
      ├── .vscode/
      │   ├── launch.json
      │   ├── tasks.json

   Verify with:
   .. code-block:: powershell

      dir
      dir src
      dir tests
      dir .vscode

3. **Build the Project**:
   Compile the project to resolve dependencies:
   .. code-block:: powershell

      cargo build

   Expected output: ``Finished dev profile [unoptimized + debuginfo]``.

4. **Run All Tests**:
   Execute both unit and integration tests:
   .. code-block:: powershell

      cargo test --all

   Expected output: All ~51 tests (~39 unit tests, 12 integration tests) pass, e.g., ``test result: ok. 39 passed`` and ``test result: ok. 12 passed``.

5. **Run Specific Tests**:
   - Run only unit tests:
     .. code-block:: powershell

        cargo test --lib
   - Run only integration tests:
     .. code-block:: powershell

        cargo test --test integration

.. uml::
   :caption: Test Execution Workflow

   @startuml
   actor User
   participant "Cargo" as Cargo
   participant "Unit Tests" as UnitTests
   participant "Integration Tests" as IntTests
   participant "TestClass1" as TC1
   participant "TestClass2" as TC2
   participant "TestClass3" as TC3

   User -> Cargo: cargo test --all
   Cargo -> UnitTests: Run unit tests
   UnitTests -> TC1: Execute tests
   UnitTests -> TC2: Execute tests
   UnitTests -> TC3: Execute tests
   UnitTests --> Cargo: Results
   Cargo -> IntTests: Run integration tests
   IntTests -> TC1: Execute tests
   IntTests -> TC2: Execute tests
   IntTests -> TC3: Execute tests
   IntTests --> Cargo: Results
   Cargo --> User: Test results
   @enduml

Applying to Other Sources
-------------------------

.. need:: Specification
   :id: SPEC_APPLY
   :title: Applying to Other Sources
   :status: implemented
   :tags: usage; tutorial; adaptation
   :links: REQ_TUTORIAL
   :description: Instructions for adapting the Rust Proxy Demonstrator to other Rust projects or data sources.

The `rust_proxy` system is modular and can be adapted to other Rust projects or data sources by modifying class implementations or extending the test suite.

1. **Modify Class Implementations**:
   - Edit `src/class1.rs`, `src/class2.rs`, or `src/class3.rs` to adapt methods like `execute`, `transform`, or `process` to new requirements.
   - Example: Modify `Class3::process` to read from a file:
     .. code-block:: rust

        fn process(&mut self, x: i32) -> i32 {
            let data = std::fs::read_to_string("data.txt").unwrap().parse::<i32>().unwrap();
            x.wrapping_mul(data).wrapping_add(self.offset)
        }
   - Update `src/test_class*.rs` to reflect changes in testing logic.

2. **Extend Test Suite**:
   - Add new unit tests in `src/lib.rs` or integration tests in `tests/integration.rs`.
   - Example: Add a unit test for a modified `Class1::execute`:
     .. code-block:: rust

        #[rstest]
        fn unit_execute_custom(tracker: TestTracker) {
            let c1 = class1::Class1::new();
            let mut tc1 = test_class1::TestClass1::new(
                c1.clone(),
                test_class1::InstrumentedClass1::new(c1.clone(), Some(tracker.clone())),
            );
            let result = tc1.execute(5);
            assert_eq!(result, /* expected value */);
            assert_eq!(tc1.get_counter(), 1);
            assert!(tracker.call_stack.is_empty());
        }

3. **Update Dependencies**:
   - Add new crates to `Cargo.toml` if required:
     .. code-block:: toml

        [dependencies]
        mockall = "0.13.0"
        new_crate = "1.0"  # Example

        [dev-dependencies]
        rstest = "0.18.0"
   - Fetch dependencies:
     .. code-block:: powershell

        cargo build

4. **Integrate into Another Project**:
   - Copy `src/` and `tests/` to a new Rust project:
     .. code-block:: powershell

        cargo new new_project
        cd new_project
        Copy-Item -Recurse -Path D:\Temp\Qorix\Proxy Test\RUST\rust_proxy\src -Destination src
        Copy-Item -Recurse -Path D:\Temp\Qorix\Proxy Test\RUST\rust_proxy\tests -Destination tests
   - Update `Cargo.toml` and adapt files to the new project’s needs.
   - Run tests:
     .. code-block:: powershell

        cargo test --all