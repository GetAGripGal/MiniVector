use crate::{gfx::electron::ElectronParams, processor::InstructionProcessor, result::Result};
use log::LevelFilter;

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;
use winit::{
    dpi::PhysicalSize,
    event::{Event, WindowEvent},
    event_loop::{EventLoop, EventLoopWindowTarget},
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

    processor: InstructionProcessor,
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
            config.instruction_per_frame as usize,
            &ElectronParams {
                radius: config.radius,
                dim_factor: config.dim_factor,
            },
        )?;
        electron_renderer.set_display_colors(&wgpu_state, config.primary, config.secondary)?;

        let processor = InstructionProcessor::new(config.instruction_per_frame as usize);

        Ok(Self {
            config,
            event_loop: Some(event_loop),
            window,
            frame_timer: FrameTimer::new(),
            wgpu_state,
            frame_buffer,
            electron_renderer,
            processor,
        })
    }

    /// Run minivector
    #[cfg_attr(target_arch = "wasm32", wasm_bindgen)]
    pub async fn run(mut self) -> Result<()> {
        log::info!("Running minivector");

        // Take the event loop from the context
        let event_loop = self.event_loop.take().unwrap();
        event_loop
            .run(|event, window_target| self.handle_winit_event(&event, window_target))
            .map_err(|e| anyhow::anyhow!("Error running event loop: {:?}", e))?;

        Ok(())
    }

    /// Handle a winit event
    fn handle_winit_event(&mut self, event: &Event<()>, window_target: &EventLoopWindowTarget<()>) {
        match event {
            Event::WindowEvent { event, window_id } if *window_id == self.window.id() => {
                self.handle_window_event(event, window_target);
            }
            Event::AboutToWait { .. } => {}
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
            }
            WindowEvent::RedrawRequested => {
                self.frame_timer.start();

                // Process the instructions
                self.processor.process();

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
                self.frame_timer.update();
                let delay = self.frame_timer.delay(self.config.frame_rate);
                std::thread::sleep(delay);

                self.frame_timer.update();
                // Update window title to reflect timing
                self.window.set_title(&format!(
                    "minivector | frame time: {}ms | fps: {}",
                    self.frame_timer.delta(),
                    self.frame_timer.fps()
                ));

                self.window.request_redraw();
            }
            WindowEvent::CloseRequested => window_target.exit(),
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
