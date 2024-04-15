use crate::events::Event;
use crate::result::Result;

/// Writes events to a named pipe
#[cfg(unix)]
pub struct NamedPipeWriter {
    file: i32,
}

/// Writes events to a named pipe
#[cfg(windows)]
pub struct NamedPipeWriter {
    file: winapi::um::winnt::HANDLE,
}

impl NamedPipeWriter {
    /// Create a new named pipe writer
    #[cfg(unix)]
    pub fn new(path: &str) -> Result<Self> {
        use nix::sys::stat::Mode;

        // Open the pipe
        let file = nix::fcntl::open(
            path,
            nix::fcntl::OFlag::O_WRONLY | nix::fcntl::OFlag::O_NONBLOCK,
            Mode::empty(),
        )
        .map_err(|e| anyhow::anyhow!("Failed to open named event pipe: {}", e))?;

        Ok(Self { file })
    }

    #[cfg(windows)]
    pub fn new(path: &str) -> Result<Self> {
        let pipe = create_pipe(path, 1)?;

        let file =
            unsafe { std::fs::File::from_raw_handle(pipe as std::os::windows::io::RawHandle) };

        Ok(Self { file })
    }

    /// Write events to the named pipe
    #[cfg(unix)]
    pub fn write(&self, events: &[Event]) -> Result<()> {
        use nix::errno::Errno;

        let bytes = events
            .iter()
            .flat_map(|event| event.to_bytes())
            .collect::<Vec<u8>>();

        // Check if the file descriptor is valid
        if self.file < 0 {
            return Err(anyhow::anyhow!("Invalid file descriptor").into());
        }

        // Borrow the file descriptor
        // This is safe because the file descriptor is valid
        let fd_borrowed = unsafe { std::os::unix::io::BorrowedFd::borrow_raw(self.file) };

        match nix::unistd::write(fd_borrowed, &bytes) {
            Ok(_) => {}
            Err(Errno::EAGAIN) => {
                return Err(anyhow::anyhow!("Named pipe is full").into());
            }
            Err(e) => {
                return Err(anyhow::anyhow!("Failed to write to named pipe: {}", e).into());
            }
        }

        Ok(())
    }

    #[cfg(windows)]
    pub fn write(&self, events: &[Event]) -> Result<()> {
        let bytes = events
            .iter()
            .flat_map(|event| event.to_bytes())
            .collect::<Vec<u8>>();

        let mut bytes_written = 0;
        let result = unsafe {
            winapi::um::fileapi::WriteFile(
                self.file,
                bytes.as_ptr() as *const std::ffi::c_void,
                bytes.len() as u32,
                &mut bytes_written,
                std::ptr::null_mut(),
            )
        };

        if result == 0 {
            return Err(anyhow::anyhow!("Failed to write to named pipe"));
        }

        Ok(())
    }
}
