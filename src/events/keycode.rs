/// Key code type
pub struct KeyCode(u64);

impl From<u32> for KeyCode {
    fn from(code: u32) -> Self {
        Self(code as u64)
    }
}

impl From<u64> for KeyCode {
    fn from(code: u64) -> Self {
        Self(code)
    }
}

impl Into<u64> for KeyCode {
    fn into(self) -> u64 {
        self.0
    }
}
