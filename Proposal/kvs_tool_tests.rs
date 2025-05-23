use std::process::{Command, Output};
use std::fs;
use std::thread;
use std::time::Duration;

// Helper function to run kvs_tool with given arguments and capture output
fn run_kvs_tool(args: Vec<&str>) -> Output {
    Command::new("./target/debug/kvs_tool")
        .args(args)
        .output()
        .expect("Failed to execute kvs_tool. Ensure `cargo build --bin kvs_tool` has been run.")
}

// Helper function to clean up test files with retries
fn cleanup_test_files() {
    let files = vec![
        "kvs_0_0.json",
        "kvs_0_0.hash",
        "kvs_0_1.json",
        "kvs_0_1.hash",
        "kvs_0_2.json",
        "kvs_0_2.hash",
        "kvs_0_3.json",
        "kvs_0_3.hash",
        "kvs_0_default.json",
    ];
    for file in files {
        for _ in 0..5 { // Increased retries
            if fs::remove_file(file).is_ok() {
                break;
            }
            thread::sleep(Duration::from_millis(100)); // Increased delay
        }
    }
    // Ensure cleanup is complete
    thread::sleep(Duration::from_millis(500)); // Increased sleep
}

#[test]
fn test_getkey_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "getkey", "-k", "MyKey"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(!output.status.success(), "Expected getkey to fail for non-existent key: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stderr.contains("Error") || stdout.contains("Key 'MyKey' does not exist!"), 
        "Expected error message in stdout or stderr: stdout: {}, stderr: {}", stdout, stderr);
}

#[test]
fn test_setkey_operation_string() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Setkey failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Parsed as String Value: Hello World"), 
        "Expected string value confirmation: stdout: {}", stdout);
}

#[test]
fn test_setkey_operation_json() {
    cleanup_test_files();
    let output = run_kvs_tool(vec![
        "-o",
        "setkey",
        "-k",
        "MyKey",
        "-p",
        r#"{"sub-number":789,"sub-string":"Third"}"#,
    ]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Setkey JSON failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Parsed as JSON Value: Object"), 
        "Expected JSON value confirmation: stdout: {}", stdout);
}

#[test]
fn test_removekey_operation() {
    cleanup_test_files();
    // First set a key
    let set_output = run_kvs_tool(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"]);
    assert!(set_output.status.success(), "Failed to set key for removekey test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&set_output.stdout), String::from_utf8_lossy(&set_output.stderr));
    // Allow time for file system to update
    thread::sleep(Duration::from_millis(2000));
    let output = run_kvs_tool(vec!["-o", "removekey", "-k", "MyKey"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(output.status.success(), "Removekey failed: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Remove Key MyKey"), 
        "Expected remove confirmation: stdout: {}, stderr: {}", stdout, stderr);
}

#[test]
fn test_listkeys_operation() {
    cleanup_test_files();
    // Reset KVS to ensure clean state
    let reset_output = run_kvs_tool(vec!["-o", "reset"]);
    assert!(reset_output.status.success(), "Failed to reset KVS for listkeys test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&reset_output.stdout), String::from_utf8_lossy(&reset_output.stderr));
    thread::sleep(Duration::from_millis(2000));
    // Set Key1
    let set1_output = run_kvs_tool(vec!["-o", "setkey", "-k", "Key1", "-p", "Value1"]);
    assert!(set1_output.status.success(), "Failed to set Key1 for listkeys test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&set1_output.stdout), String::from_utf8_lossy(&set1_output.stderr));
    // Verify Key1 is set
    let get1_output = run_kvs_tool(vec!["-o", "getkey", "-k", "Key1"]);
    assert!(get1_output.status.success(), "Failed to verify Key1 for listkeys test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&get1_output.stdout), String::from_utf8_lossy(&get1_output.stderr));
    // Allow time for file system to update
    thread::sleep(Duration::from_millis(2000));
    // Set Key2
    let set2_output = run_kvs_tool(vec!["-o", "setkey", "-k", "Key2", "-p", "Value2"]);
    assert!(set2_output.status.success(), "Failed to set Key2 for listkeys test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&set2_output.stdout), String::from_utf8_lossy(&set2_output.stderr));
    // Verify Key2 is set
    let get2_output = run_kvs_tool(vec!["-o", "getkey", "-k", "Key2"]);
    assert!(get2_output.status.success(), "Failed to verify Key2 for listkeys test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&get2_output.stdout), String::from_utf8_lossy(&get2_output.stderr));
    // Allow time for file system to update
    thread::sleep(Duration::from_millis(2000));
    // List keys
    let output = run_kvs_tool(vec!["-o", "listkeys"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(output.status.success(), "Listkeys failed: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Key1"), "Expected Key1 in output: stdout: {}, stderr: {}", stdout, stderr);
    assert!(stdout.contains("Key2"), "Expected Key2 in output: stdout: {}, stderr: {}", stdout, stderr);
}

#[test]
fn test_reset_operation() {
    cleanup_test_files();
    // Retry setting a key up to 3 times
    let mut set_output = None;
    for _ in 0..3 {
        let output = run_kvs_tool(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"]);
        if output.status.success() {
            set_output = Some(output);
            break;
        }
        thread::sleep(Duration::from_millis(500)); // Wait before retry
    }
    let set_output = set_output.expect("Failed to set key for reset test after retries");
    let stdout = String::from_utf8_lossy(&set_output.stdout);
    let stderr = String::from_utf8_lossy(&set_output.stderr);
    assert!(set_output.status.success(), "Failed to set key for reset test: stdout: {}, stderr: {}", stdout, stderr);
    // Allow time for file system to update
    thread::sleep(Duration::from_millis(2000));
    let output = run_kvs_tool(vec!["-o", "reset"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Reset failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Reset KVS"), "Expected reset confirmation: stdout: {}", stdout);
}

#[test]
fn test_snapshotcount_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "snapshotcount"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Snapshotcount failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Snapshot Count:"), "Expected snapshot count: stdout: {}", stdout);
}

#[test]
fn test_snapshotmaxcount_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "snapshotmaxcount"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Snapshotmaxcount failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Snapshots Maximum Count:"), "Expected max count: stdout: {}", stdout);
}

#[test]
fn test_snapshotrestore_operation() {
    cleanup_test_files();
    // Create a key to ensure KVS is initialized
    let set_output = run_kvs_tool(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"]);
    assert!(set_output.status.success(), "Failed to set key for snapshotrestore test: stdout: {}, stderr: {}", 
        String::from_utf8_lossy(&set_output.stdout), String::from_utf8_lossy(&set_output.stderr));
    // Allow time for file system to update
    thread::sleep(Duration::from_millis(2000));
    let output = run_kvs_tool(vec!["-o", "snapshotrestore", "-s", "1"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);
    // Check if operation fails due to missing snapshot or succeeds
    if output.status.success() {
        assert!(stdout.contains("Snapshot Restore"), "Expected restore confirmation: stdout: {}, stderr: {}", stdout, stderr);
    } else {
        assert!(stderr.contains("KVS restore failed"), "Expected restore error: stdout: {}, stderr: {}", stdout, stderr);
    }
}

#[test]
fn test_getkvsfilename_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "getkvsfilename", "-s", "1"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Getkvsfilename failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("KVS Filename: kvs_0_1.json"), "Expected filename: stdout: {}", stdout);
}

#[test]
fn test_gethashfilename_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "gethashfilename", "-s", "1"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Gethashfilename failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Hash Filename: kvs_0_1.hash"), "Expected hash filename: stdout: {}", stdout);
}

#[test]
fn test_createtestdata_operation() {
    cleanup_test_files();
    let output = run_kvs_tool(vec!["-o", "createtestdata"]);
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(output.status.success(), "Createtestdata failed: stdout: {}, stderr: {}", stdout, String::from_utf8_lossy(&output.stderr));
    assert!(stdout.contains("Create Test Data"), "Expected test data creation: stdout: {}", stdout);
    assert!(stdout.contains("Done!"), "Expected completion: stdout: {}", stdout);
}