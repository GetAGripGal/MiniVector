use std::fmt::Debug;

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

/// Cross-platform result type for minivector
pub type Result<T> = std::result::Result<T, Box<dyn MvError>>;

/// Cross-platform error handling for minivector
pub trait MvError {
    fn error(&self) -> String;
}

impl Debug for dyn MvError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.error())
    }
}

impl MvError for anyhow::Error {
    fn error(&self) -> String {
        self.to_string()
    }
}

impl From<anyhow::Error> for Box<dyn MvError> {
    fn from(error: anyhow::Error) -> Self {
        Box::new(error)
    }
}

#[cfg(target_arch = "wasm32")]
impl Into<JsValue> for Box<dyn MvError> {
    fn into(self) -> JsValue {
        JsValue::from_str(&self.error())
    }
}
