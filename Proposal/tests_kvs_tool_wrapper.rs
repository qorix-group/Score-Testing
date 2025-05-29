use mockall::mock;
use rust_kvs::{ErrorCode, KvsValue};
use std::collections::HashMap;
use std::ffi::OsString;
use tinyjson::JsonValue;

// Mocked Kvs trait without generics for dyn safety
trait KvsTrait {
    fn key_exists(&self, key: &str) -> Result<bool, ErrorCode>;
    fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode>;
    fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
    fn get_value_string(&self, key: &str) -> Result<String, ErrorCode>;
    fn set_value(&self, key: &str, value: KvsValue) -> Result<(), ErrorCode>;
    fn remove_key(&self, key: &str) -> Result<(), ErrorCode>;
    fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode>;
    fn reset(&self) -> Result<(), ErrorCode>;
    fn snapshot_count(&self) -> usize;
    fn snapshot_restore(&self, id: u32) -> Result<(), ErrorCode>;
    fn get_kvs_filename(&self, id: u32) -> String;
    fn get_hash_filename(&self, id: u32) -> String;
}

mock! {
    KvsMock {}
    impl KvsTrait for KvsMock {
        fn key_exists(&self, key: &str) -> Result<bool, ErrorCode>;
        fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode>;
        fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
        fn get_value_string(&self, key: &str) -> Result<String, ErrorCode>;
        fn set_value(&self, key: &str, value: KvsValue) -> Result<(), ErrorCode>;
        fn remove_key(&self, key: &str) -> Result<(), ErrorCode>;
        fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode>;
        fn reset(&self) -> Result<(), ErrorCode>;
        fn snapshot_count(&self) -> usize;
        fn snapshot_restore(&self, id: u32) -> Result<(), ErrorCode>;
        fn get_kvs_filename(&self, id: u32) -> String;
        fn get_hash_filename(&self, id: u32) -> String;
    }
}

// Wrapper to invoke kvs_tool operations
struct KvsToolWrapper {
    kvs: Box<dyn KvsTrait>,
}

impl KvsToolWrapper {
    fn new(kvs: Box<dyn KvsTrait>) -> Self {
        KvsToolWrapper { kvs }
    }

    fn execute_operation(&self, args: Vec<&str>) -> Result<(), ErrorCode> {
        let args: Vec<OsString> = args.into_iter().map(|s| s.into()).collect();
        let mut pico_args = pico_args::Arguments::from_vec(args);

        if pico_args.contains(["-h", "--help"]) {
            return Ok(());
        }

        let operation: Option<String> = pico_args
            .opt_value_from_str(["-o", "--operation"])
            .map_err(|_| ErrorCode::UnmappedError)?;
        let op_mode = match operation.as_deref() {
            Some("getkey") => "getkey",
            Some("setkey") => "setkey",
            Some("removekey") => "removekey",
            Some("listkeys") => "listkeys",
            Some("reset") => "reset",
            Some("snapshotcount") => "snapshotcount",
            Some("snapshotmaxcount") => "snapshotmaxcount",
            Some("snapshotrestore") => "snapshotrestore",
            Some("getkvsfilename") => "getkvsfilename",
            Some("gethashfilename") => "gethashfilename",
            Some("createtestdata") => "createtestdata",
            _ => return Err(ErrorCode::UnmappedError),
        };

        match op_mode {
            "getkey" => {
                let key: String = pico_args
                    .opt_value_from_str(["-k", "--key"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                if !self.kvs.key_exists(&key)? {
                    return Err(ErrorCode::KeyNotFound);
                }
                let _ = self.kvs.is_value_default(&key)?;
                let _ = self.kvs.get_default_value(&key)?;
                let _ = self.kvs.get_value_string(&key)?;
                Ok(())
            }
            "setkey" => {
                let key: String = pico_args
                    .opt_value_from_str(["-k", "--key"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                let value: String = pico_args
                    .opt_value_from_str(["-p", "--payload"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                let kvs_value = if let Ok(json) = value.parse::<JsonValue>() {
                    convert_json_to_kvs(&json)
                } else {
                    KvsValue::String(value)
                };
                self.kvs.set_value(&key, kvs_value)?;
                Ok(())
            }
            "removekey" => {
                let key: String = pico_args
                    .opt_value_from_str(["-k", "--key"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                self.kvs.remove_key(&key)?;
                Ok(())
            }
            "listkeys" => {
                self.kvs.get_all_keys()?;
                Ok(())
            }
            "reset" => {
                self.kvs.reset()?;
                Ok(())
            }
            "snapshotcount" => {
                self.kvs.snapshot_count();
                Ok(())
            }
            "snapshotmaxcount" => {
                Ok(())
            }
            "snapshotrestore" => {
                let snapshot_id: u32 = pico_args
                    .opt_value_from_str(["-s", "--snapshotid"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                self.kvs.snapshot_restore(snapshot_id)?;
                Ok(())
            }
            "getkvsfilename" => {
                let snapshot_id: u32 = pico_args
                    .opt_value_from_str(["-s", "--snapshotid"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                self.kvs.get_kvs_filename(snapshot_id);
                Ok(())
            }
            "gethashfilename" => {
                let snapshot_id: u32 = pico_args
                    .opt_value_from_str(["-s", "--snapshotid"])
                    .map_err(|_| ErrorCode::UnmappedError)?
                    .ok_or(ErrorCode::UnmappedError)?;
                self.kvs.get_hash_filename(snapshot_id);
                Ok(())
            }
            "createtestdata" => {
                self.kvs.set_value("number", KvsValue::Number(123.0))?;
                self.kvs.set_value("bool", KvsValue::Boolean(true))?;
                self.kvs.set_value("string", KvsValue::String("First".to_string()))?;
                self.kvs.set_value("null", KvsValue::Null)?;
                self.kvs.set_value(
                    "array",
                    KvsValue::Array(vec![
                        KvsValue::Number(456.0),
                        KvsValue::Boolean(false),
                        KvsValue::String("Second".to_string()),
                    ]),
                )?;
                self.kvs.set_value(
                    "object",
                    KvsValue::Object(HashMap::from([
                        ("sub-number".to_string(), KvsValue::Number(789.0)),
                        ("sub-bool".to_string(), KvsValue::Boolean(true)),
                        ("sub-string".to_string(), KvsValue::String("Third".to_string())),
                        ("sub-null".to_string(), KvsValue::Null),
                        (
                            "sub-array".to_string(),
                            KvsValue::Array(vec![
                                KvsValue::Number(1246.0),
                                KvsValue::Boolean(false),
                                KvsValue::String("Fourth".to_string()),
                            ]),
                        ),
                    ])),
                )?;
                Ok(())
            }
            _ => Err(ErrorCode::UnmappedError),
        }
    }
}

// Convert tinyjson::JsonValue to KvsValue
fn convert_json_to_kvs(json: &JsonValue) -> KvsValue {
    match json {
        JsonValue::Number(n) => KvsValue::Number(*n),
        JsonValue::Boolean(b) => KvsValue::Boolean(*b),
        JsonValue::String(s) => KvsValue::String(s.clone()),
        JsonValue::Null => KvsValue::Null,
        JsonValue::Array(arr) => KvsValue::Array(arr.iter().map(convert_json_to_kvs).collect()),
        JsonValue::Object(obj) => KvsValue::Object(
            obj.iter()
                .map(|(k, v)| (k.clone(), convert_json_to_kvs(v)))
                .collect(),
        ),
    }
}

// Integration tests for transitions
#[test]
fn test_getkey_non_existent() {
    let mut mock = MockKvsMock::new();
    mock.expect_key_exists()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok(false));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    let result = wrapper.execute_operation(vec!["-o", "getkey", "-k", "MyKey"]);
    assert!(result.is_err());
}

#[test]
fn test_setkey_getkey_transition() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "MyKey" && matches!(value, KvsValue::String(s) if s == "Hello World")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_key_exists()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok(true));
    mock.expect_is_value_default()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok(false));
    mock.expect_get_default_value()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok(KvsValue::String("Default".to_string())));
    mock.expect_get_value_string()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok("Hello World".to_string()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"])
        .is_ok());
    assert!(wrapper
        .execute_operation(vec!["-o", "getkey", "-k", "MyKey"])
        .is_ok());
}

#[test]
fn test_setkey_json_transition() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "MyKey"
                && matches!(
                    value,
                    KvsValue::Object(hm) if hm.get("sub-number").map(|v| matches!(v, KvsValue::Number(n) if *n == 789.0)).unwrap_or(false)
                )
        })
        .times(1)
        .returning(|_, _| Ok(()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec![
            "-o",
            "setkey",
            "-k",
            "MyKey",
            "-p",
            r#"{"sub-number":789,"sub-string":"Third"}"#
        ])
        .is_ok());
}

#[test]
fn test_setkey_removekey_transition() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "MyKey" && matches!(value, KvsValue::String(s) if s == "Hello World")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_remove_key()
        .withf(|key: &str| key == "MyKey")
        .times(1)
        .returning(|_| Ok(()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"])
        .is_ok());
    assert!(wrapper
        .execute_operation(vec!["-o", "removekey", "-k", "MyKey"])
        .is_ok());
}

#[test]
fn test_setkey_listkeys_transition() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "Key1" && matches!(value, KvsValue::String(s) if s == "Value1")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "Key2" && matches!(value, KvsValue::String(s) if s == "Value2")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_get_all_keys()
        .times(1)
        .returning(|| Ok(vec!["Key1".to_string(), "Key2".to_string()]));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "setkey", "-k", "Key1", "-p", "Value1"])
        .is_ok());
    assert!(wrapper
        .execute_operation(vec!["-o", "setkey", "-k", "Key2", "-p", "Value2"])
        .is_ok());
    assert!(wrapper.execute_operation(vec!["-o", "listkeys"]).is_ok());
}

#[test]
fn test_setkey_reset_transition() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "MyKey" && matches!(value, KvsValue::String(s) if s == "Hello World")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_reset().times(1).returning(|| Ok(()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"])
        .is_ok());
    assert!(wrapper.execute_operation(vec!["-o", "reset"]).is_ok());
}

#[test]
fn test_snapshotcount() {
    let mut mock = MockKvsMock::new();
    mock.expect_snapshot_count().times(1).returning(|| 2);

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper.execute_operation(vec!["-o", "snapshotcount"]).is_ok());
}

#[test]
fn test_snapshotmaxcount() {
    let wrapper = KvsToolWrapper::new(Box::new(MockKvsMock::new()));
    assert!(wrapper.execute_operation(vec!["-o", "snapshotmaxcount"]).is_ok());
}

#[test]
fn test_snapshotrestore() {
    let mut mock = MockKvsMock::new();
    mock.expect_snapshot_restore()
        .withf(|id: &u32| *id == 1)
        .times(1)
        .returning(|_| Ok(()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "snapshotrestore", "-s", "1"])
        .is_ok());
}

#[test]
fn test_getkvsfilename() {
    let mut mock = MockKvsMock::new();
    mock.expect_get_kvs_filename()
        .withf(|id: &u32| *id == 1)
        .times(1)
        .returning(|id| format!("kvs_0_{}.json", id));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "getkvsfilename", "-s", "1"])
        .is_ok());
}

#[test]
fn test_gethashfilename() {
    let mut mock = MockKvsMock::new();
    mock.expect_get_hash_filename()
        .withf(|id: &u32| *id == 1)
        .times(1)
        .returning(|id| format!("kvs_0_{}.hash", id));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "gethashfilename", "-s", "1"])
        .is_ok());
}

#[test]
fn test_createtestdata() {
    let mut mock = MockKvsMock::new();
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "number" && matches!(value, KvsValue::Number(n) if n == &123.0)
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "bool" && matches!(value, KvsValue::Boolean(b) if *b)
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "string" && matches!(value, KvsValue::String(s) if s == "First")
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| key == "null" && matches!(value, KvsValue::Null))
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "array"
                && matches!(
                    value,
                    KvsValue::Array(vec) if vec.len() == 3
                        && matches!(vec[0], KvsValue::Number(n) if n == 456.0)
                        && matches!(vec[1], KvsValue::Boolean(b) if !b)
                        && matches!(&vec[2], KvsValue::String(s) if s == "Second")
                )
        })
        .times(1)
        .returning(|_, _| Ok(()));
    mock.expect_set_value()
        .withf(|key: &str, value: &KvsValue| {
            key == "object"
                && matches!(
                    value,
                    KvsValue::Object(hm) if hm.len() == 5
                        && hm.get("sub-number").map(|v| matches!(v, KvsValue::Number(n) if n == &789.0)).unwrap_or(false)
                        && hm.get("sub-bool").map(|v| matches!(v, KvsValue::Boolean(b) if *b)).unwrap_or(false)
                        && hm.get("sub-string").map(|v| matches!(v, KvsValue::String(s) if s == "Third")).unwrap_or(false)
                        && hm.get("sub-null").map(|v| matches!(v, KvsValue::Null)).unwrap_or(false)
                        && hm.get("sub-array").map(|v| matches!(v, KvsValue::Array(vec) if vec.len() == 3
                            && matches!(vec[0], KvsValue::Number(n) if n == 1246.0)
                            && matches!(vec[1], KvsValue::Boolean(b) if !b)
                            && matches!(&vec[2], KvsValue::String(s) if s == "Fourth")
                        )).unwrap_or(false)
                )
        })
        .times(1)
        .returning(|_, _| Ok(()));

    let wrapper = KvsToolWrapper::new(Box::new(mock));
    assert!(wrapper
        .execute_operation(vec!["-o", "createtestdata"])
        .is_ok());
}