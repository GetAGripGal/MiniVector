use crate::{instruction::INSTRUCTION_SIZE, result::Result};

#[cfg(windows)]
use std::fs::File;

/// Named pipe reader
#[derive(Debug)]
#[cfg(unix)]
pub struct NamedPipeReader {
    file: i32,
    instructions_per_frame: usize,
    left_over: Vec<u8>, // The left over bytes from the last read. If the last instruction is incomplete, it will be stored here.
}

/// Named pipe reader
#[derive(Debug)]
#[cfg(windows)]
pub struct NamedPipeReader {
    path: String,
    pipe: winapi::um::winnt::HANDLE,
    file: File,
    instructions_per_frame: usize,
    left_over: Vec<u8>, // The left over bytes from the last read. If the last instruction is incomplete, it will be stored here.
}

unsafe impl Send for NamedPipeReader {}
unsafe impl Sync for NamedPipeReader {}

impl NamedPipeReader {
    /// Create a new named pipe reader
    #[cfg(unix)]
    pub fn new(path: &str, instructions_per_frame: usize) -> Result<NamedPipeReader> {
        use nix::{fcntl::OFlag, sys::stat::Mode};

        create_pipe(path, instructions_per_frame)?;
        log::info!("Named pipe created: {}", path);

        // Open the pipe non-blocking
        let file = nix::fcntl::open(path, OFlag::O_RDONLY | OFlag::O_NONBLOCK, Mode::empty())
            .map_err(|e| anyhow::anyhow!("Failed to open named instruction pipe: {}", e))?;

        Ok(Self {
            file,
            instructions_per_frame,
            left_over: Vec::new(),
        })
    }

    /// Create a new named pipe reader
    #[cfg(windows)]
    pub fn new(path: &str, instructions_per_frame: usize) -> Result<NamedPipeReader> {
        use std::os::windows::io::FromRawHandle;
        let pipe = create_pipe(path, instructions_per_frame)?;

        // Open the pipe non-blocking
        let file =
            unsafe { std::fs::File::from_raw_handle(pipe as std::os::windows::io::RawHandle) };

        Ok(Self {
            path: path.to_string(),
            pipe,
            file,
            instructions_per_frame,
            left_over: Vec::new(),
        })
    }

    /// Read instructions from the named pipe
    #[cfg(unix)]
    pub fn read(&mut self) -> Result<Vec<u8>> {
        // Read from the named pipe
        let mut bytes_read = 0;
        let mut buffer = [0; 1];
        let capacity = if self.instructions_per_frame > 0 {
            self.instructions_per_frame * INSTRUCTION_SIZE
        } else {
            std::mem::size_of::<u8>() * 1024
        };
        let mut bytes = Vec::with_capacity(capacity);
        loop {
            match nix::unistd::read(self.file, &mut buffer) {
                Ok(n) => {
                    if n <= 0 {
                        break;
                    }
                    bytes.push(buffer[0]);
                    bytes_read += n;
                    // Break if we have read the required number of instructions
                    if self.instructions_per_frame > 0
                        && bytes_read >= self.instructions_per_frame * INSTRUCTION_SIZE
                    {
                        break;
                    }
                }
                Err(e) => {
                    if e == nix::errno::Errno::EAGAIN {
                        break;
                    } else {
                        return Err(anyhow::anyhow!("Failed to read from named pipe: {}", e).into());
                    }
                }
            }
        }

        // Truncate the buffer to the number of bytes read
        bytes.truncate(bytes_read);

        // If the buffer is not a multiple of the instruction size, store the left over bytes
        let left_over_amount = bytes.len() % INSTRUCTION_SIZE;
        let split_point = bytes.len() - left_over_amount;

        // Split the buffer at the split point
        let (bytes, left_over) = bytes.split_at(split_point);

        // Store the left over bytes
        self.left_over.clear();
        self.left_over.extend_from_slice(&left_over);

        Ok(bytes.to_vec())
    }

    #[cfg(windows)]
    pub fn read(&mut self) -> Result<Vec<u8>> {
        use std::io::Error;
        use std::os::windows::io::AsRawHandle;
        use winapi::um::fileapi::ReadFile;
        use winapi::um::minwinbase::OVERLAPPED;
        use winapi::um::winnt::HANDLE;

        // Create a buffer to store the instructions
        let mut buffer = vec![0; self.instructions_per_frame * INSTRUCTION_SIZE];
        let mut bytes_read: u32 = 0;
        let mut overlapped: OVERLAPPED = unsafe { std::mem::zeroed() };

        // Read from the named pipe
        let result = unsafe {
            ReadFile(
                self.file.as_raw_handle() as HANDLE,
                buffer.as_mut_ptr() as *mut _,
                buffer.len() as u32,
                &mut bytes_read,
                &mut overlapped,
            )
        };

        // Truncate the buffer to the number of bytes read
        buffer.truncate(bytes_read as usize);

        // If the buffer is not a multiple of the instruction size, store the left over bytes
        let left_over_amount = buffer.len() % INSTRUCTION_SIZE;
        let split_point = buffer.len() - left_over_amount;

        // Split the buffer at the split point
        let (buffer, left_over) = buffer.split_at(split_point);
        if left_over_amount > 0 {
            log::info!("Left over: {}", left_over_amount);
        }

        // Store the left over bytes
        self.left_over.clear();
        self.left_over.extend_from_slice(left_over);

        Ok(buffer.to_vec())
    }
}

impl Drop for NamedPipeReader {
    fn drop(&mut self) {
        #[cfg(unix)]
        nix::unistd::close(self.file).unwrap();

        #[cfg(windows)]
        unsafe {
            winapi::um::handleapi::CloseHandle(self.pipe);
        }
    }
}

/// Create a named pipe
#[cfg(unix)]
pub fn create_pipe(pipe: &str, _: usize) -> Result<()> {
    use nix::sys::stat;

    let path = std::path::Path::new(pipe);
    if !path.exists() {
        nix::unistd::mkfifo(path, stat::Mode::empty())
            .map_err(|e| anyhow::anyhow!("Failed to create instruction pipe{}", e))?;
    }

    Ok(())
}

/// Create a named pipe
#[cfg(windows)]
pub fn create_pipe(pipe: &str, instructions_per_frame: usize) -> Result<winapi::um::winnt::HANDLE> {
    use std::os::windows::ffi::OsStrExt;
    use std::{ffi::OsString, fs};

    use crate::instruction::INSTRUCTION_SIZE;

    let path = std::path::Path::new(pipe);
    let path_os_str: OsString = path.as_os_str().into();
    let path_slice = path_os_str.encode_wide().collect::<Vec<u16>>();

    let instruction_buffer_size =
        (instructions_per_frame * INSTRUCTION_SIZE * std::mem::size_of::<u8>()) as u32;

    let mut pipe = unsafe {
        winapi::um::namedpipeapi::CreateNamedPipeW(
            path_slice.as_ptr(),
            winapi::um::winbase::PIPE_ACCESS_INBOUND | winapi::um::winbase::FILE_FLAG_OVERLAPPED,
            winapi::um::winbase::PIPE_TYPE_BYTE | winapi::um::winbase::PIPE_READMODE_BYTE,
            2,
            instruction_buffer_size,
            instruction_buffer_size,
            0,
            std::ptr::null_mut(),
        )
    };

    if pipe == winapi::um::handleapi::INVALID_HANDLE_VALUE {
        return Err(anyhow::anyhow!("Failed to create instruction pipe").into());
    }

    log::info!(
        "Named pipe created: {}",
        path_slice.iter().collect::<String>()
    );

    Ok(pipe)
}
