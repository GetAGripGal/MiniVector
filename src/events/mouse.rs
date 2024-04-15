/// Mouse button
pub struct MouseButton(u64);

impl MouseButton {
    pub const LEFT: Self = Self(0);
    pub const RIGHT: Self = Self(1);
    pub const MIDDLE: Self = Self(2);
    pub const BACK: Self = Self(3);
    pub const FORWARD: Self = Self(4);
}

impl Into<u64> for MouseButton {
    fn into(self) -> u64 {
        self.0
    }
}

// Convert a winit mouse button to a minivector mouse button
impl From<winit::event::MouseButton> for MouseButton {
    fn from(button: winit::event::MouseButton) -> Self {
        match button {
            winit::event::MouseButton::Left => Self::LEFT,
            winit::event::MouseButton::Right => Self::RIGHT,
            winit::event::MouseButton::Middle => Self::MIDDLE,
            winit::event::MouseButton::Back => Self::BACK,
            winit::event::MouseButton::Forward => Self::FORWARD,
            winit::event::MouseButton::Other(button) => Self(button as u64),
        }
    }
}
