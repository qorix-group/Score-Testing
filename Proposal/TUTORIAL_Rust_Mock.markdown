# Tutorial: Applying Integration Testing to Other Modules

## Introduction

This tutorial explains how to apply the integration testing approach used in `tests/kvs_tool_tests.rs` to other Rust modules, particularly when the original code cannot be modified. The `kvs_tool` tests avoided traditional mocking (e.g., using `mockall`) due to the inability to modify `src/kvs_tool.rs` or `src/lib.rs`. Instead, they used a command-line testing approach, running the binary and verifying output. This tutorial adapts this approach for other modules, with guidance on incorporating mocking when possible.

## Prerequisites
- **Rust Toolchain**: Ensure a stable toolchain (e.g., `stable-x86_64-pc-windows-gnu`) is installed:
  ```powershell
  rustup install stable-x86_64-pc-windows-gnu
  rustup default stable-x86_64-pc-windows-gnu
  ```
- **Project Structure**: A Rust project with a binary or library module to test, configured in `Cargo.toml`.
- **Dependencies**: For mocking (if applicable), add `mockall` to `[dev-dependencies]` in `Cargo.toml`:
  ```toml
  [dev-dependencies]
  mockall = "0.13"
  ```

## Step-by-Step Guide

### 1. Understand the Module
- **Identify the Interface**: Determine the public interface of the module to test (e.g., command-line arguments for a binary, public functions for a library).
  - For `kvs_tool`, the interface was the command-line arguments (`-o getkey`, `-k MyKey`, etc.).
- **Check Constraints**: Confirm if the code can be modified. If not, focus on integration testing via the public interface, as done with `kvs_tool`.
- **Dependencies**: Identify external dependencies (e.g., file system, network, or libraries like `Kvs`). For `kvs_tool`, the `Kvs` store was a file-based dependency.

### 2. Choose Testing Approach
- **Integration Testing (No Modification)**:
  - Use when code cannot be modified, as with `kvs_tool`.
  - Run the binary or call public functions, verifying output or side effects (e.g., files created).
  - Example: `tests/kvs_tool_tests.rs` runs `./target/debug/kvs_tool` and checks `stdout`/`stderr`.
- **Mocking (With Modification)**:
  - Use if you can modify the code to inject dependencies (e.g., via traits or dependency injection).
  - Define a trait for the dependency (e.g., `KvsTrait` for `Kvs`), implement it for the real dependency, and use `mockall` to create a mock version.
- **Hybrid Approach**:
  - If partial modification is allowed, mock specific dependencies while testing the public interface.

For this tutorial, we’ll focus on the **integration testing approach** used for `kvs_tool`, with notes on adapting it for mocking.

### 3. Set Up Integration Tests
- **Create Test File**: Place tests in the `tests/` directory (e.g., `tests/my_module_tests.rs`).
- **Configure Cargo**: Ensure the module is a binary or library in `Cargo.toml`. For a binary:
  ```toml
  [[bin]]
  name = "my_module"
  path = "src/my_module.rs"
  ```
- **Dependencies**: Add `std::process::Command` for running binaries and `std::fs` for file cleanup (used in `kvs_tool` tests).

### 4. Write Integration Tests
Follow the pattern from `tests/kvs_tool_tests.rs`:

#### Example: Testing a Hypothetical `my_module` Binary
Assume `my_module` is a command-line tool with operations like `add-user` and `list-users`, storing data in files (`users_*.json`).

```rust
use std::process::{Command, Output};
use std::fs;
use std::thread;
use std::time::Duration;

// Helper function to run my_module
fn run_my_module(args: Vec<&str>) -> Output {
    Command::new("./target/debug/my_module")
        .args(args)
        .output()
        .expect("Failed to execute my_module. Ensure `cargo build --bin my_module` has been run.")
}

// Helper function to clean up test files
fn cleanup_test_files() {
    let files = vec!["users_0.json", "users_1.json"];
    for file in files {
        for _ in 0..3 {
            if fs::remove_file(file).is_ok() {
                break;
            }
            thread::sleep(Duration::from_millis(50));
        }
    }
    thread::sleep(Duration::from_millis(200));
}

#[test]
fn test_add_user() {
    cleanup_test_files();
    let output = run_my_module(vec!["-o", "add-user", "-n", "Alice"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(output.status.success(), "Add-user failed: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Added user: Alice"), "Expected confirmation: stdout: {}, stderr: {}", stdout, stderr);
}

#[test]
fn test_list_users() {
    cleanup_test_files();
    // Reset to clean state
    let reset_output = run_my_module(vec!["-o", "reset"]);
    assert!(reset_output.status.success(), "Reset failed: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&reset_output.stdout), String::from_utf8_lossy(&reset_output.stderr));
    thread::sleep(Duration::from_millis(2000));
    // Add users
    let add1_output = run_my_module(vec!["-o", "add-user", "-n", "Alice"]);
    assert!(add1_output.status.success(), "Failed to add Alice: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&add1_output.stdout), String::from_utf8_lossy(&add1_output.stderr));
    thread::sleep(Duration::from_millis(2000));
    let add2_output = run_my_module(vec!["-o", "add-user", "-n", "Bob"]);
    assert!(add2_output.status.success(), "Failed to add Bob: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&add2_output.stdout), String::from_utf8_lossy(&add2_output.stderr));
    thread::sleep(Duration::from_millis(2000));
    // List users
    let output = run_my_module(vec!["-o", "list-users"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(output.status.success(), "List-users failed: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Alice"), "Expected Alice in output: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Bob"), "Expected Bob in output: stdout: {}, stderr: {}", stdout, stderr);
}
```

#### Key Elements
- **Cleanup**: The `cleanup_test_files` function removes module-specific files, retrying to handle file system delays.
- **Timing**: Use `thread::sleep(Duration::from_millis(2000))` for operations modifying state (e.g., `add-user`), mirroring `kvs_tool`’s approach to Windows file system delays.
- **Reset**: Include a `reset` operation to clear state before setting test data, preventing interference.
- **Assertions**: Verify exit status and output strings, including debug output for diagnostics.

### 5. Incorporating Mocking (If Modification Allowed)
If you can modify the module’s code, you can use mocking to isolate dependencies:

#### Example: Mocking a File-Based Store
Assume `my_module` uses a `UserStore` struct similar to `Kvs`.

1. **Define a Trait** (requires modifying `my_module.rs`):
   ```rust
   pub trait UserStoreTrait {
       fn add_user(&self, name: &str) -> Result<(), Error>;
       fn list_users(&self) -> Result<Vec<String>, Error>;
   }

   impl UserStoreTrait for UserStore {
       fn add_user(&self, name: &str) -> Result<(), Error> { /* real impl */ }
       fn list_users(&self) -> Result<Vec<String>, Error> { /* real impl */ }
   }
   ```

2. **Update Module**: Modify `my_module.rs` to accept a `&dyn UserStoreTrait` instead of `UserStore`.

3. **Write Mock Tests**:
   ```rust
   use mockall::mock;

   mock! {
       UserStoreMock {}
       impl UserStoreTrait for UserStoreMock {
           fn add_user(&self, name: &str) -> Result<(), Error>;
           fn list_users(&self) -> Result<Vec<String>, Error>;
       }
   }

   #[test]
   fn test_add_user_mock() {
       let mut mock = MockUserStoreMock::new();
       mock.expect_add_user()
           .withf(|name: &str| name == "Alice")
           .times(1)
           .returning(|_| Ok(()));
       let result = my_module::add_user(&mock, "Alice");
       assert!(result.is_ok());
   }
   ```

4. **Apply to Integration Tests**:
   - If modification is not allowed, continue using the command-line approach, but mock external systems (e.g., file system) if possible using crates like `tempfile`.
   - For `kvs_tool`, mocking was avoided due to the no-modification constraint, but the principle applies: isolate the dependency’s effects (e.g., files) and verify the interface.

### 6. Handle Common Challenges
- **File System Dependencies**:
  - Use `cleanup_test_files` with retries and sleep, as in `kvs_tool`.
  - Example: Remove `users_*.json` files and wait 200ms.
- **Timing Issues**:
  - Add `thread::sleep(Duration::from_millis(2000))` after operations modifying state, especially on Windows.
- **State Interference**:
  - Include a `reset` operation or equivalent to clear state before tests.
  - Verify state with read operations (e.g., `getkey` in `kvs_tool`).
- **No Modification**:
  - Focus on public interfaces (command-line or exported functions).
  - Verify side effects (files, output) instead of internal state.

### 7. Run and Debug Tests
- **Build Binary**:
  ```powershell
  cargo build --bin my_module
  ```
- **Run Tests**:
  ```powershell
  cargo test
  ```
- **Debug Failures**:
  - Use `RUST_BACKTRACE=1 cargo test` for detailed errors.
  - Check `stdout`/`stderr` in assertions for unexpected output.
  - Verify file system state:
    ```powershell
    dir users_*
    ```

### 8. Document Tests
- **Sphinx Needs**: Document test cases using Sphinx Needs notation, as done for `kvs_tool` (see `test_specifications.rst`).
- **UML Diagrams**: Use PlantUML to visualize test flows, showing interactions between the test script, binary, and dependencies.
- **Example**:
  ```restructuredtext
  .. need:: TEST_ADD_USER
     :title: Test Add User
     :description: Verifies adding a user to my_module.
     :tags: integration, my_module

     .. uml::
        @startuml
        actor Tester
        participant "Test Script" as Test
        participant "my_module" as Module
        Tester -> Test: Run test_add_user
        Test -> Module: Run ./target/debug/my_module -o add-user -n Alice
        Module --> Test: stdout: "Added user: Alice", status: zero
        Test -> Tester: Assert zero status, check confirmation
        @enduml
  ```

## Conclusion
The integration testing approach from `kvs_tool` is adaptable to other Rust modules, especially when code modification is restricted. By focusing on the public interface, managing file system dependencies, and handling timing issues, you can create robust tests. When modification is allowed, incorporate mocking using traits and `mockall` to isolate dependencies, enhancing test granularity.