use std::collections::VecDeque;

const FRAME_TIME_BUFFER_SIZE: usize = 100;

/// Calculate the frame time.
pub struct FrameTimer {
    start_time: instant::Instant,
    delta: instant::Duration,
    frame_times: VecDeque<f64>,
}

impl FrameTimer {
    /// Create a new frame timer
    pub fn new() -> Self {
        Self {
            start_time: instant::Instant::now(),
            delta: instant::Duration::from_secs(0),
            frame_times: VecDeque::with_capacity(FRAME_TIME_BUFFER_SIZE),
        }
    }

    /// Start the frame timer
    pub fn start(&mut self) {
        self.start_time = instant::Instant::now();
    }

    /// Update the frame timer
    pub fn update(&mut self) {
        let frame_end = instant::Instant::now();
        self.delta = frame_end - self.start_time;

        self.frame_times.push_back(self.delta.as_secs_f64());
        if self.frame_times.len() > FRAME_TIME_BUFFER_SIZE {
            self.frame_times.pop_front();
        }
    }

    /// Get the delta time
    pub fn delta(&self) -> u128 {
        self.delta.as_millis()
    }

    /// Get the delta time as seconds
    pub fn delta_seconds(&self) -> f64 {
        self.delta.as_secs_f64()
    }

    /// Get the frames per second
    pub fn fps(&self) -> u32 {
        (1.0 / self.delta_seconds()).round() as u32
    }

    /// Calculate the delay between frames
    pub fn delay(&self, target_fps: u32) -> instant::Duration {
        let target_frame_time = instant::Duration::from_secs_f64(1.0 / target_fps as f64);
        if self.delta < target_frame_time {
            target_frame_time - self.delta
        } else {
            instant::Duration::from_secs(0)
        }
    }
}
