#[cfg(test)]
mod tests {
    use super::*;
    use mockall::mock;
    use pico_args::Arguments;
    use rust_kvs::{ErrorCode, InstanceId, KvsValue, OpenNeedDefaults, OpenNeedKvs, SnapshotId};
    use std::collections::HashMap;

    // Define a trait that mirrors the Kvs methods used in kvs_tool.rs
    trait KvsTrait {
        fn key_exists(&self, key: &str) -> Result<bool, ErrorCode>;
        fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode>;
        fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
        fn get_value<T: for<'a> TryFrom<&'a KvsValue> + Clone>(
            &self,
            key: &str,
        ) -> Result<T, ErrorCode>
        where
            for<'a> <T as TryFrom<&'a KvsValue>>::Error: std::fmt::Debug;
        fn set_value<S: Into<String>, J: Into<KvsValue>>(
            &self,
            key: S,
            value: J,
        ) -> Result<(), ErrorCode>;
        fn remove_key(&self, key: &str) -> Result<(), ErrorCode>;
        fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode>;
        fn reset(&self) -> Result<(), ErrorCode>;
        fn snapshot_count(&self) -> usize;
        fn snapshot_restore(&self, id: SnapshotId) -> Result<(), ErrorCode>;
        fn get_kvs_filename(&self, id: SnapshotId) -> String;
        fn get_hash_filename(&self, id: SnapshotId) -> String;
    }

    // Implement KvsTrait for Kvs (for real usage)
    impl KvsTrait for Kvs {
        fn key_exists(&self, key: &str) -> Result<bool, ErrorCode> {
            self.key_exists(key)
        }
        fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode> {
            self.is_value_default(key)
        }
        fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode> {
            self.get_default_value(key)
        }
        fn get_value<T: for<'a> TryFrom<&'a KvsValue> + Clone>(
            &self,
            key: &str,
        ) -> Result<T, ErrorCode>
        where
            for<'a> <T as TryFrom<&'a KvsValue>>::Error: std::fmt::Debug,
        {
            self.get_value(key)
        }
        fn set_value<S: Into<String>, J: Into<KvsValue>>(
            &self,
            key: S,
            value: J,
        ) -> Result<(), ErrorCode> {
            self.set_value(key, value)
        }
        fn remove_key(&self, key: &str) -> Result<(), ErrorCode> {
            self.remove_key(key)
        }
        fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode> {
            self.get_all_keys()
        }
        fn reset(&self) -> Result<(), ErrorCode> {
            self.reset()
        }
        fn snapshot_count(&self) -> usize {
            self.snapshot_count()
        }
        fn snapshot_restore(&self, id: SnapshotId) -> Result<(), ErrorCode> {
            self.snapshot_restore(id)
        }
        fn get_kvs_filename(&self, id: SnapshotId) -> String {
            self.get_kvs_filename(id)
        }
        fn get_hash_filename(&self, id: SnapshotId) -> String {
            self.get_hash_filename(id)
        }
    }

    // Create a mock for KvsTrait
    mock! {
        KvsMock {}
        impl KvsTrait for KvsMock {
            fn key_exists(&self, key: &str) -> Result<bool, ErrorCode>;
            fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode>;
            fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
            fn get_value<T: for<'a> TryFrom<&'a KvsValue> + Clone>(
                &self,
                key: &str,
            ) -> Result<T, ErrorCode>
            where
                for<'a> <T as TryFrom<&'a KvsValue>>::Error: std::fmt::Debug;
            fn set_value<S: Into<String>, J: Into<KvsValue>>(
                &self,
                key: S,
                value: J,
            ) -> Result<(), ErrorCode>;
            fn remove_key(&self, key: &str) -> Result<(), ErrorCode>;
            fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode>;
            fn reset(&self) -> Result<(), ErrorCode>;
            fn snapshot_count(&self) -> usize;
            fn snapshot_restore(&self, id: SnapshotId) -> Result<(), ErrorCode>;
            fn get_kvs_filename(&self, id: SnapshotId) -> String;
            fn get_hash_filename(&self, id: SnapshotId) -> String;
        }
    }

    // Helper function to create Arguments from a vector of strings
    fn create_args(args: Vec<&str>) -> Arguments {
        let mut vec: Vec<String> = args.into_iter().map(|s| s.to_string()).collect();
        Arguments::from_vec(vec)
    }

    #[test]
    fn test_getkey_operation() {
        let mut mock = MockKvsMock::new();
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
        mock.expect_get_value::<String>()
            .withf(|key: &str| key == "MyKey")
            .times(1)
            .returning(|_| Ok("Hello".to_string()));

        let args = create_args(vec!["-o", "getkey", "-k", "MyKey"]);
        let result = _getkey(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_setkey_operation_string() {
        let mut mock = MockKvsMock::new();
        mock.expect_set_value()
            .withf(|key: &str, value: &KvsValue| {
                key == "MyKey" && matches!(value, KvsValue::String(s) if s == "Hello World")
            })
            .times(1)
            .returning(|_, _| Ok(()));

        let args = create_args(vec!["-o", "setkey", "-k", "MyKey", "-p", "Hello World"]);
        let result = _setkey(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_setkey_operation_json() {
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

        let args = create_args(vec![
            "-o",
            "setkey",
            "-k",
            "MyKey",
            "-p",
            r#"{"sub-number":789,"sub-string":"Third"}"#,
        ]);
        let result = _setkey(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_removekey_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_remove_key()
            .withf(|key: &str| key == "MyKey")
            .times(1)
            .returning(|_| Ok(()));

        let args = create_args(vec!["-o", "removekey", "-k", "MyKey"]);
        let result = _removekey(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_listkeys_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_get_all_keys()
            .times(1)
            .returning(|| Ok(vec!["Key1".to_string(), "Key2".to_string()]));

        let result = _listkeys(&mock);
        assert!(result.is_ok());
    }

    #[test]
    fn test_reset_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_reset().times(1).returning(|| Ok(()));

        let result = _reset(&mock);
        assert!(result.is_ok());
    }

    #[test]
    fn test_snapshotcount_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_snapshot_count().times(1).returning(|| 2);

        let result = _snapshotcount(&mock);
        assert!(result.is_ok());
    }

    #[test]
    fn test_snapshotmaxcount_operation() {
        let result = _snapshotmaxcount();
        assert!(result.is_ok());
    }

    #[test]
    fn test_snapshotrestore_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_snapshot_restore()
            .withf(|id: &SnapshotId| id.0 == 1)
            .times(1)
            .returning(|_| Ok(()));

        let args = create_args(vec!["-o", "snapshotrestore", "-s", "1"]);
        let result = _snapshotrestore(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_getkvsfilename_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_get_kvs_filename()
            .withf(|id: &SnapshotId| id.0 == 1)
            .times(1)
            .returning(|id| format!("kvs_0_{}.json", id.0));

        let args = create_args(vec!["-o", "getkvsfilename", "-s", "1"]);
        let result = _getkvsfilename(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_gethashfilename_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_get_hash_filename()
            .withf(|id: &SnapshotId| id.0 == 1)
            .times(1)
            .returning(|id| format!("kvs_0_{}.hash", id.0));

        let args = create_args(vec!["-o", "gethashfilename", "-s", "1"]);
        let result = _gethashfilename(&mock, args);
        assert!(result.is_ok());
    }

    #[test]
    fn test_createtestdata_operation() {
        let mut mock = MockKvsMock::new();
        mock.expect_set_value()
            .withf(|key: &str, value: &KvsValue| {
                key == "number" && matches!(value, KvsValue::Number(n) if *n == 123.0)
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
                            && matches!(vec[2], KvsValue::String(s) if s == "Second")
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
                            && hm.get("sub-number").map(|v| matches!(v, KvsValue::Number(n) if *n == 789.0)).unwrap_or(false)
                            && hm.get("sub-bool").map(|v| matches!(v, KvsValue::Boolean(b) if *b)).unwrap_or(false)
                            && hm.get("sub-string").map(|v| matches!(v, KvsValue::String(s) if s == "Third")).unwrap_or(false)
                            && hm.get("sub-null").map(|v| matches!(v, KvsValue::Null)).unwrap_or(false)
                            && hm.get("sub-array").map(|v| matches!(v, KvsValue::Array(vec) if vec.len() == 3
                                && matches!(vec[0], KvsValue::Number(n) if *n == 1246.0)
                                && matches!(vec[1], KvsValue::Boolean(b) if !b)
                                && matches!(vec[2], KvsValue::String(s) if s == "Fourth")
                            )).unwrap_or(false)
                    )
            })
            .times(1)
            .returning(|_, _| Ok(()));

        let result = _createtestdata(&mock);
        assert!(result.is_ok());
    }
}