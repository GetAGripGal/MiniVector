use std::sync::{Arc, Mutex};

use crate::{
    events::{
        manager::EventManager,
        writer::{pipe::NamedPipeWriter, EventWriter},
        Event,
    },
    gfx::{electron::ElectronParams, point::Point},
    instruction,
    processor::InstructionProcessor,
    readers::{pipe::NamedPipeReader, InstructionReader},
    result::Result,
};
use instruction::Instruction;
use log::LevelFilter;

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;
use winit::{
    dpi::PhysicalSize,
    event::WindowEvent,
    event_loop::{EventLoop, EventLoopWindowTarget},
    platform::scancode::PhysicalKeyExtScancode,
    window::{Fullscreen, Icon, Window, WindowBuilder},
};

use crate::{
    config::Config,
    gfx::{electron::ElectronRenderer, framebuffer::FrameBuffer, WgpuState},
    timing::FrameTimer,
};

/// The minivector context
#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
pub struct Context {
    config: Config,
    event_loop: Option<EventLoop<()>>,
    frame_timer: FrameTimer,

    wgpu_state: WgpuState,
    frame_buffer: FrameBuffer,
    electron_renderer: ElectronRenderer,
    window: Window,

    reader: Option<Arc<Mutex<InstructionReader>>>,
    processor: InstructionProcessor,

    event_manager: EventManager,
    event_writer: Option<EventWriter>,

    reading_thread: Option<std::thread::JoinHandle<()>>,
    reading_thread_handle: Arc<Mutex<bool>>,

    instruction_buffer: Arc<Mutex<Vec<instruction::Instruction>>>,
}

#[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
impl Context {
    /// Initialize the state
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen(constructor))]
    pub async fn new(config: Config) -> Result<Context> {
        init_logger();

        let (event_loop, window) = init_window(&config)?;

        #[cfg(target_arch = "wasm32")]
        attach_canvas(&window, &config)?;

        // # Safety
        // The surface does not live longer than the window
        let wgpu_state =
            unsafe { WgpuState::new(&window, (config.window.width, config.window.height)).await }?;

        let frame_buffer = FrameBuffer::new(
            &wgpu_state,
            (config.resolution.width, config.resolution.height),
        );
        let mut electron_renderer = ElectronRenderer::new(
            &wgpu_state,
            &frame_buffer,
            (config.instruction_per_frame + 1) as usize,
            &ElectronParams {
                current_point: (0.0, 0.0).into(),
                radius: config.radius,
                dim_factor: config.dim_factor,
                screen_size: (config.screen_size.x, config.screen_size.y).into(),
            },
            config.resolution.into(),
        )?;
        electron_renderer.set_display_colors(&wgpu_state, config.primary, config.secondary)?;

        let processor = InstructionProcessor::new(config.instruction_per_frame as usize);

        // Determine the reader
        let reader = {
            if let Some(pipe) = &config.instruction_pipe {
                let result = InstructionReader::NamedPipe(NamedPipeReader::new(
                    pipe,
                    config.instruction_per_frame as usize,
                )?);
                Some(Arc::new(Mutex::new(result)))
            } else {
                None
            }
        };

        // Create the event manager
        let event_manager = EventManager::new();

        // Determine the event writer
        let event_writer = {
            if let Some(pipe) = &config.event_pipe {
                let result = EventWriter::NamedPipe(NamedPipeWriter::new(&pipe)?);
                Some(result)
            } else {
                None
            }
        };

        Ok(Self {
            config,
            event_loop: Some(event_loop),
            window,
            frame_timer: FrameTimer::new(),
            wgpu_state,
            frame_buffer,
            electron_renderer,
            processor,
            reader,
            reading_thread: None,
            reading_thread_handle: Arc::new(Mutex::new(true)),
            instruction_buffer: Arc::new(Mutex::new(Vec::new())),
            event_manager,
            event_writer,
        })
    }

    /// Run minivector
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
    pub async fn run(mut self) -> Result<()> {
        log::info!("Running minivector");

        // Start the reader thread
        if let Some(_) = &self.reader {
            self.spawn_reader_thread();
        }
        // Take the event loop from the context
        let event_loop = self.event_loop.take().unwrap();
        event_loop
            .run(|event, window_target| self.handle_winit_event(&event, window_target))
            .map_err(|e| anyhow::anyhow!("Error running event loop: {:?}", e))?;

        self.reading_thread.take().unwrap().join().unwrap();
        Ok(())
    }

    /// Spawn the reading thread
    fn spawn_reader_thread(&mut self) {
        let handle = self.reading_thread_handle.clone();
        let reader = self.reader.as_ref().unwrap().clone();
        let instruction_buffer = self.instruction_buffer.clone();
        let mut thread_instruction_buffer: Vec<Instruction> = Vec::new();

        // Spawn the reading thread
        self.reading_thread = Some(std::thread::spawn(move || loop {
            // Check if the handle is set to false. If so, exit the thread
            if let Ok(handle) = handle.try_lock() {
                if !*handle {
                    break;
                }
            }

            // Read instructions and push them to the buffer
            if let Ok(mut reader) = reader.try_lock() {
                // Read instructions from the reader
                let buffer = reader.read().unwrap();
                if buffer.is_empty() {
                    continue;
                }
                match Instruction::from_bytes_slice(&buffer) {
                    Ok(instructions) => {
                        thread_instruction_buffer.extend(&instructions);
                    }
                    Err(e) => log::error!("Failed to parse instructions: {:?}", e),
                }
            }

            // Push the instructions to the instruction buffer
            if thread_instruction_buffer.is_empty() {
                continue;
            }
            if let Ok(mut instruction_buffer) = instruction_buffer.try_lock() {
                instruction_buffer.extend(&thread_instruction_buffer);
                thread_instruction_buffer.clear();
            }
        }));
    }

    /// Handle a winit event
    fn handle_winit_event(
        &mut self,
        event: &winit::event::Event<()>,
        window_target: &EventLoopWindowTarget<()>,
    ) {
        match event {
            winit::event::Event::WindowEvent { event, window_id }
                if *window_id == self.window.id() =>
            {
                self.handle_window_event(event, window_target);
            }
            winit::event::Event::AboutToWait { .. } => {
                // Dispatch the events
                let queue = self.event_manager.queue();
                let handlers = self.event_manager.handlers();
                for event in queue {
                    if let Some(handler) = handlers.get(&event.kind) {
                        handler(self, &event);
                    }

                    if let Some(event_writer) = &mut self.event_writer {
                        event_writer.write(&[event.clone()]).ok();
                    }
                }
                self.event_manager.clear();
            }
            _ => (),
        }
    }

    /// Handle a window event
    fn handle_window_event(
        &mut self,
        event: &WindowEvent,
        window_target: &EventLoopWindowTarget<()>,
    ) {
        match event {
            WindowEvent::Resized(size) => {
                self.wgpu_state.resize((*size).into());
                self.frame_buffer
                    .update_aspect(&self.wgpu_state, size.width as f32 / size.height as f32);
            }
            WindowEvent::RedrawRequested => {
                self.frame_timer.start();

                // Process the instructions
                {
                    let mut instruction_buffer = self.instruction_buffer.lock().unwrap();
                    self.processor.push_instructions(&instruction_buffer);
                    instruction_buffer.clear();
                }
                self.processor.process();

                // // Clear if the processor requests it
                // if self.processor.should_clear() {
                //     self.electron_renderer.clear(&mut self.wgpu_state);
                // }

                // Update the electron renderer
                self.electron_renderer
                    .set_points(&self.wgpu_state, self.processor.points())
                    .unwrap();
                // Process the electron renderer
                self.electron_renderer
                    .process(&mut self.wgpu_state, &self.frame_buffer);

                self.frame_buffer
                    .render(&mut self.wgpu_state, self.config.primary)
                    .unwrap();

                // Limit the frame rate
                if self.config.frame_rate > 0 {
                    self.frame_timer.update();
                    let delay = self.frame_timer.delay(self.config.frame_rate);
                    std::thread::sleep(delay);
                }

                self.frame_timer.update();
                // Update window title to reflect timing
                self.window.set_title(&format!(
                    "minivector | frame time: {}ms | fps: {}",
                    self.frame_timer.delta(),
                    self.frame_timer.fps()
                ));

                self.window.request_redraw();
                self.event_manager.dispatch(Event::frame_finished());
            }
            WindowEvent::MouseInput { state, button, .. } => {
                let event = match state {
                    winit::event::ElementState::Pressed => Event::mouse_pressed((*button).into()),
                    winit::event::ElementState::Released => Event::mouse_released((*button).into()),
                };
                self.event_manager.dispatch(event);
            }
            WindowEvent::KeyboardInput { event, .. } => {
                let scancode = event.physical_key.to_scancode();
                if let Some(code) = scancode {
                    // If f11 is pressed, toggle fullscreen
                    if code == 87 && event.state == winit::event::ElementState::Pressed {
                        if self.window.fullscreen().is_some() {
                            self.window.set_fullscreen(None);
                        } else {
                            self.window
                                .set_fullscreen(Some(Fullscreen::Borderless(None)));
                        }
                    }
                    let event = match event.state {
                        winit::event::ElementState::Pressed => Event::key_pressed(code.into()),
                        winit::event::ElementState::Released => Event::key_released(code.into()),
                    };
                    self.event_manager.dispatch(event);
                }
            }
            WindowEvent::CloseRequested => {
                log::info!("Exiting minivector");
                *self.reading_thread_handle.lock().unwrap() = false;
                log::info!("Reading thread exited");
                window_target.exit();
            }
            _ => (),
        }
    }

    /// Push instructions to the instruction processor
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
    pub fn push_instructions(&mut self, instructions: &[u8]) {
        let instructions = crate::instruction::Instruction::from_bytes_slice(instructions)
            .expect("Failed to convert instructions");
        self.processor.push_instructions(&instructions);
    }
}

/// Initialize the window
fn init_window(config: &Config) -> anyhow::Result<(EventLoop<()>, Window)> {
    let event_loop = EventLoop::new()?;
    #[cfg(not(target_arch = "wasm32"))]
    let icon = Icon::from_rgba(
        config.icon.bytes.clone(),
        config.icon.width,
        config.icon.height,
    )
    .ok();
    #[cfg(target_arch = "wasm32")]
    let icon = None;

    let window = WindowBuilder::new()
        .with_title("minivector")
        .with_inner_size(PhysicalSize::new(config.window.width, config.window.height))
        .with_resizable(false)
        .with_window_icon(icon)
        .with_fullscreen(if config.fullscreen {
            Some(Fullscreen::Borderless(None))
        } else {
            None
        })
        .build(&event_loop)?;

    Ok((event_loop, window))
}

/// Attach the window to the canvas
#[cfg(target_arch = "wasm32")]
fn attach_canvas(window: &winit::window::Window, config: &Config) -> anyhow::Result<()> {
    use winit::platform::web::WindowExtWebSys;

    const MINIVECTOR_BODY_ID: &str = "minivector";

    let web_window = web_sys::window().ok_or(anyhow::anyhow!("Failed to get window"))?;
    let document = web_window
        .document()
        .ok_or(anyhow::anyhow!("Failed to get document"))?;
    let body = document
        .get_element_by_id(MINIVECTOR_BODY_ID)
        .ok_or(anyhow::anyhow!(
            "Failed to get body element with id: {}",
            MINIVECTOR_BODY_ID
        ))?;

    let canvas = window
        .canvas()
        .ok_or(anyhow::anyhow!("Failed to get canvas"))?;
    canvas.set_width(config.window.width);
    canvas.set_height(config.window.height);
    body.append_child(&canvas)
        .map_err(|_| anyhow::anyhow!("Failed to append canvas to body"))?;

    Ok(())
}

/// Initialize the minivector logger
fn init_logger() {
    #[cfg(not(target_arch = "wasm32"))]
    {
        env_logger::Builder::from_default_env()
            .filter_level(LevelFilter::Info)
            .init();
    }
    #[cfg(target_arch = "wasm32")]
    {
        // Enable logging for wasm
        console_log::init_with_level(log::Level::Info).expect("Failed to initialize logger");

        console_error_panic_hook::set_once();
    }
}
