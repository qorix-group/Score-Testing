.. need:: TEST_001
   :title: Test Getkey Operation
   :status: implemented
   :description: Verifies that the `getkey` operation correctly handles a non-existent key by returning an error status and appropriate error message.
   :tags: integration, kvs_tool, getkey
   :links: REQ_001

   **Test Steps**:
   1. Clean up KVS files to ensure a fresh state.
   2. Run `kvs_tool` with arguments `-o getkey -k MyKey`.
   3. Verify the command exits with a non-zero status (failure).
   4. Check that `stdout` contains "Key 'MyKey' does not exist!" or `stderr` contains "Error".

   **Expected Result**:
   - Command exits with non-zero status.
   - Error message confirms the key does not exist.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_getkey_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o getkey -k MyKey
      KvsTool -> Kvs: key_exists("MyKey")
      Kvs --> KvsTool: false
      KvsTool -> Kvs: is_value_default("MyKey")
      Kvs --> KvsTool: false
      KvsTool --> Test: stdout: "Key 'MyKey' does not exist!", stderr: "Error", status: non-zero
      Test -> Tester: Assert non-zero status, check error message
      @enduml

.. need:: TEST_002
   :title: Test Setkey Operation (String)
   :status: implemented
   :description: Verifies that the `setkey` operation correctly sets a string value and outputs confirmation.
   :tags: integration, kvs_tool, setkey
   :links: REQ_002

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o setkey -k MyKey -p "Hello World"`.
   3. Verify the command exits with zero status (success).
   4. Check that `stdout` contains "Parsed as String Value: Hello World".

   **Expected Result**:
   - Command exits with zero status.
   - Output confirms the string value was set.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_setkey_operation_string
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k MyKey -p "Hello World"
      KvsTool -> Kvs: set_value("MyKey", String("Hello World"))
      Kvs --> KvsTool: Ok(())
      KvsTool --> Test: stdout: "Parsed as String Value: Hello World", status: zero
      Test -> Tester: Assert zero status, check confirmation
      @enduml

.. need:: TEST_003
   :title: Test Setkey Operation (JSON)
   :status: implemented
   :description: Verifies that the `setkey` operation correctly sets a JSON object value and outputs confirmation.
   :tags: integration, kvs_tool, setkey
   :links: REQ_002

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o setkey -k MyKey -p '{"sub-number":789,"sub-string":"Third"}'`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "Parsed as JSON Value: Object".

   **Expected Result**:
   - Command exits with zero status.
   - Output confirms the JSON object was set.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_setkey_operation_json
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k MyKey -p '{"sub-number":789,"sub-string":"Third"}'
      KvsTool -> Kvs: set_value("MyKey", Object({...}))
      Kvs --> KvsTool: Ok(())
      KvsTool --> Test: stdout: "Parsed as JSON Value: Object", status: zero
      Test -> Tester: Assert zero status, check confirmation
      @enduml

.. need:: TEST_004
   :title: Test Removekey Operation
   :status: implemented
   :description: Verifies that the `removekey` operation removes an existing key and outputs confirmation.
   :tags: integration, kvs_tool, removekey
   :links: REQ_003

   **Test Steps**:
   1. Clean up KVS files.
   2. Set a key with `kvs_tool -o setkey -k MyKey -p "Hello World"`.
   3. Wait 2000ms for persistence.
   4. Run `kvs_tool` with arguments `-o removekey -k MyKey`.
   5. Verify the command exits with zero status.
   6. Check that `stdout` contains "Remove Key MyKey".

   **Expected Result**:
   - Command exits with zero status.
   - Output confirms the key was removed.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_removekey_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k MyKey -p "Hello World"
      KvsTool -> Kvs: set_value("MyKey", String("Hello World"))
      Kvs --> KvsTool: Ok(())
      Test -> Test: sleep(2000ms)
      Test -> KvsTool: Run ./target/debug/kvs_tool -o removekey -k MyKey
      KvsTool -> Kvs: remove_key("MyKey")
      Kvs --> KvsTool: Ok(())
      KvsTool --> Test: stdout: "Remove Key MyKey", status: zero
      Test -> Tester: Assert zero status, check confirmation
      @enduml

.. need:: TEST_005
   :title: Test Listkeys Operation
   :status: implemented
   :description: Verifies that the `listkeys` operation lists all keys after setting multiple keys.
   :tags: integration, kvs_tool, listkeys
   :links: REQ_004

   **Test Steps**:
   1. Clean up KVS files and reset KVS.
   2. Set key "Key1" with `kvs_tool -o setkey -k Key1 -p Value1`.
   3. Verify "Key1" with `getkey`.
   4. Wait 2000ms.
   5. Set key "Key2" with `kvs_tool -o setkey -k Key2 -p Value2`.
   6. Verify "Key2" with `getkey`.
   7. Wait 2000ms.
   8. Run `kvs_tool` with arguments `-o listkeys`.
   9. Verify the command exits with zero status.
   10. Check that `stdout` contains "Key1" and "Key2".

   **Expected Result**:
   - Command exits with zero status.
   - Output lists both keys.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_listkeys_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o reset
      KvsTool -> Kvs: reset()
      Kvs --> KvsTool: Ok(())
      Test -> Test: sleep(2000ms)
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k Key1 -p Value1
      KvsTool -> Kvs: set_value("Key1", String("Value1"))
      Kvs --> KvsTool: Ok(())
      Test -> KvsTool: Run ./target/debug/kvs_tool -o getkey -k Key1
      KvsTool -> Kvs: get_value("Key1")
      Kvs --> KvsTool: Ok(String("Value1"))
      Test -> Test: sleep(2000ms)
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k Key2 -p Value2
      KvsTool -> Kvs: set_value("Key2", String("Value2"))
      Kvs --> KvsTool: Ok(())
      Test -> KvsTool: Run ./target/debug/kvs_tool -o getkey -k Key2
      KvsTool -> Kvs: get_value("Key2")
      Kvs --> KvsTool: Ok(String("Value2"))
      Test -> Test: sleep(2000ms)
      Test -> KvsTool: Run ./target/debug/kvs_tool -o listkeys
      KvsTool -> Kvs: get_all_keys()
      Kvs --> KvsTool: Ok(["Key1", "Key2"])
      KvsTool --> Test: stdout: "Key1\nKey2", status: zero
      Test -> Tester: Assert zero status, check Key1 and Key2
      @enduml

.. need:: TEST_006
   :title: Test Reset Operation
   :status: implemented
   :description: Verifies that the `reset` operation clears all keys and outputs confirmation.
   :tags: integration, kvs_tool, reset
   :links: REQ_005

   **Test Steps**:
   1. Clean up KVS files.
   2. Set a key with `kvs_tool -o setkey -k MyKey -p "Hello World"`.
   3. Run `kvs_tool` with arguments `-o reset`.
   4. Verify the command exits with zero status.
   5. Check that `stdout` contains "Reset KVS".

   **Expected Result**:
   - Command exits with zero status.
   - Output confirms the KVS was reset.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_reset_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k MyKey -p "Hello World"
      KvsTool -> Kvs: set_value("MyKey", String("Hello World"))
      Kvs --> KvsTool: Ok(())
      Test -> KvsTool: Run ./target/debug/kvs_tool -o reset
      KvsTool -> Kvs: reset()
      Kvs --> KvsTool: Ok(())
      KvsTool --> Test: stdout: "Reset KVS", status: zero
      Test -> Tester: Assert zero status, check confirmation
      @enduml

.. need:: TEST_007
   :title: Test Snapshotcount Operation
   :status: implemented
   :description: Verifies that the `snapshotcount` operation outputs the number of snapshots.
   :tags: integration, kvs_tool, snapshotcount
   :links: REQ_006

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o snapshotcount`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "Snapshot Count:".

   **Expected Result**:
   - Command exits with zero status.
   - Output includes the snapshot count.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_snapshotcount_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o snapshotcount
      KvsTool -> Kvs: snapshot_count()
      Kvs --> KvsTool: count
      KvsTool --> Test: stdout: "Snapshot Count: <count>", status: zero
      Test -> Tester: Assert zero status, check count output
      @enduml

.. need:: TEST_008
   :title: Test Snapshotmaxcount Operation
   :status: implemented
   :description: Verifies that the `snapshotmaxcount` operation outputs the maximum number of snapshots.
   :tags: integration, kvs_tool, snapshotmaxcount
   :links: REQ_007

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o snapshotmaxcount`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "Snapshots Maximum Count:".

   **Expected Result**:
   - Command exits with zero status.
   - Output includes the maximum snapshot count.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_snapshotmaxcount_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o snapshotmaxcount
      KvsTool -> Kvs: snapshot_max_count()
      Kvs --> KvsTool: max_count
      KvsTool --> Test: stdout: "Snapshots Maximum Count: <max_count>", status: zero
      Test -> Tester: Assert zero status, check max count output
      @enduml

.. need:: TEST_009
   :title: Test Snapshotrestore Operation
   :status: implemented
   :description: Verifies that the `snapshotrestore` operation either succeeds or fails appropriately for a given snapshot ID.
   :tags: integration, kvs_tool, snapshotrestore
   :links: REQ_008

   **Test Steps**:
   1. Clean up KVS files.
   2. Set a key to initialize KVS with `kvs_tool -o setkey -k MyKey -p "Hello World"`.
   3. Wait 2000ms.
   4. Run `kvs_tool` with arguments `-o snapshotrestore -s 1`.
   5. If the command exits with zero status, check that `stdout` contains "Snapshot Restore".
   6. If the command exits with non-zero status, check that `stderr` contains "KVS restore failed".

   **Expected Result**:
   - Command exits with zero or non-zero status.
   - Output confirms restore or error.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_snapshotrestore_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o setkey -k MyKey -p "Hello World"
      KvsTool -> Kvs: set_value("MyKey", String("Hello World"))
      Kvs --> KvsTool: Ok(())
      Test -> Test: sleep(2000ms)
      Test -> KvsTool: Run ./target/debug/kvs_tool -o snapshotrestore -s 1
      KvsTool -> Kvs: snapshot_restore(SnapshotId(1))
      alt Success
        Kvs --> KvsTool: Ok(())
        KvsTool --> Test: stdout: "Snapshot Restore", status: zero
        Test -> Tester: Assert stdout contains "Snapshot Restore"
      else Failure
        Kvs --> KvsTool: Err(Error)
        KvsTool --> Test: stderr: "KVS restore failed", status: non-zero
        Test -> Tester: Assert stderr contains "KVS restore failed"
      end
      @enduml

.. need:: TEST_010
   :title: Test Getkvsfilename Operation
   :status: implemented
   :description: Verifies that the `getkvsfilename` operation outputs the correct filename for a snapshot ID.
   :tags: integration, kvs_tool, getkvsfilename
   :links: REQ_009

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o getkvsfilename -s 1`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "KVS Filename: kvs_0_1.json".

   **Expected Result**:
   - Command exits with zero status.
   - Output includes the correct filename.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_getkvsfilename_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o getkvsfilename -s 1
      KvsTool -> Kvs: get_kvs_filename(SnapshotId(1))
      Kvs --> KvsTool: "kvs_0_1.json"
      KvsTool --> Test: stdout: "KVS Filename: kvs_0_1.json", status: zero
      Test -> Tester: Assert zero status, check filename
      @enduml

.. need:: TEST_011
   :title: Test Gethashfilename Operation
   :status: implemented
   :description: Verifies that the `gethashfilename` operation outputs the correct hash filename for a snapshot ID.
   :tags: integration, kvs_tool, gethashfilename
   :links: REQ_010

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o gethashfilename -s 1`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "Hash Filename: kvs_0_1.hash".

   **Expected Result**:
   - Command exits with zero status.
   - Output includes the correct hash filename.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_gethashfilename_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o gethashfilename -s 1
      KvsTool -> Kvs: get_hash_filename(SnapshotId(1))
      Kvs --> KvsTool: "kvs_0_1.hash"
      KvsTool --> Test: stdout: "Hash Filename: kvs_0_1.hash", status: zero
      Test -> Tester: Assert zero status, check hash filename
      @enduml

.. need:: TEST_012
   :title: Test Createtestdata Operation
   :status: implemented
   :description: Verifies that the `createtestdata` operation creates test data and outputs confirmation.
   :tags: integration, kvs_tool, createtestdata
   :links: REQ_011

   **Test Steps**:
   1. Clean up KVS files.
   2. Run `kvs_tool` with arguments `-o createtestdata`.
   3. Verify the command exits with zero status.
   4. Check that `stdout` contains "Create Test Data" and "Done!".

   **Expected Result**:
   - Command exits with zero status.
   - Output confirms test data creation.

   .. uml::
      @startuml
      actor Tester
      participant "Test Script" as Test
      participant "kvs_tool" as KvsTool
      participant "KVS" as Kvs

      Tester -> Test: Run test_createtestdata_operation
      Test -> Test: cleanup_test_files()
      Test -> KvsTool: Run ./target/debug/kvs_tool -o createtestdata
      KvsTool -> Kvs: set_value("number", Number(123.0))
      Kvs --> KvsTool: Ok(())
      KvsTool -> Kvs: set_value("bool", Boolean(true))
      Kvs --> KvsTool: Ok(())
      KvsTool -> Kvs: set_value("string", String("First"))
      Kvs --> KvsTool: Ok(())
      KvsTool -> Kvs: set_value("null", Null)
      Kvs --> KvsTool: Ok(())
      KvsTool -> Kvs: set_value("array", Array([...]))
      Kvs --> KvsTool: Ok(())
      KvsTool -> Kvs: set_value("object", Object({...}))
      Kvs --> KvsTool: Ok(())
      KvsTool --> Test: stdout: "Create Test Data\nDone!", status: zero
      Test -> Tester: Assert zero status, check confirmation
      @enduml

.. need:: REQ_001
   :title: Getkey Functionality
   :status: implemented
   :description: The `kvs_tool` shall retrieve the value of a specified key, returning an error if the key does not exist.
   :tags: requirement, kvs_tool

.. need:: REQ_002
   :title: Setkey Functionality
   :status: implemented
   :description: The `kvs_tool` shall set a key with a string or JSON value and confirm the operation.
   :tags: requirement, kvs_tool

.. need:: REQ_003
   :title: Removekey Functionality
   :status: implemented
   :description: The `kvs_tool` shall remove a specified key and confirm the operation.
   :tags: requirement, kvs_tool

.. need:: REQ_004
   :title: Listkeys Functionality
   :status: implemented
   :description: The `kvs_tool` shall list all keys in the KVS.
   :tags: requirement, kvs_tool

.. need:: REQ_005
   :title: Reset Functionality
   :status: implemented
   :description: The `kvs_tool` shall reset the KVS, clearing all keys.
   :tags: requirement, kvs_tool

.. need:: REQ_006
   :title: Snapshotcount Functionality
   :status: implemented
   :description: The `kvs_tool` shall output the number of snapshots.
   :tags: requirement, kvs_tool

.. need:: REQ_007
   :title: Snapshotmaxcount Functionality
   :status: implemented
   :description: The `kvs_tool` shall output the maximum number of snapshots.
   :tags: requirement, kvs_tool

.. need:: REQ_008
   :title: Snapshotrestore Functionality
   :status: implemented
   :description: The `kvs_tool` shall restore a specified snapshot or return an error if it does not exist.
   :tags: requirement, kvs_tool

.. need:: REQ_009
   :title: Getkvsfilename Functionality
   :status: implemented
   :description: The `kvs_tool` shall output the filename for a given snapshot ID.
   :tags: requirement, kvs_tool

.. need:: REQ_010
   :title: Gethashfilename Functionality
   :status: implemented
   :description: The `kvs_tool` shall output the hash filename for a given snapshot ID.
   :tags: requirement, kvs_tool

.. need:: REQ_011
   :title: Createtestdata Functionality
   :status: implemented
   :description: The `kvs_tool` shall create predefined test data in the KVS.
   :tags: requirement, kvs_tool