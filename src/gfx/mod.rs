use winit::raw_window_handle::{HasDisplayHandle, HasWindowHandle};

pub mod display_colors;
pub mod electron;
pub mod framebuffer;
pub mod point;

/// The state of the WGPU renderer
#[derive(Debug)]
pub struct WgpuState {
    pub surface: wgpu::Surface<'static>,
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    pub config: wgpu::SurfaceConfiguration,
    pub size: (u32, u32),
}

impl WgpuState {
    /// Create a new WGPU state
    /// # Safety
    /// The surface must not live longer than the window
    pub async unsafe fn new<R>(window: &R, size: (u32, u32)) -> anyhow::Result<Self>
    where
        R: HasWindowHandle + HasDisplayHandle + std::marker::Send + std::marker::Sync,
    {
        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends: wgpu::Backends::all(),
            ..Default::default()
        });

        // # Safety
        // The surface must not live longer than the window
        let surface = unsafe {
            let target = wgpu::SurfaceTargetUnsafe::from_window(window)?;
            instance.create_surface_unsafe(target)?
        };

        let adapter = request_adapter(&instance, &surface).await?;

        let (device, queue) = request_device(&adapter).await?;

        let config = configure_surface(&adapter, &device, &surface, size);

        Ok(Self {
            surface,
            device,
            queue,
            config,
            size,
        })
    }

    /// Resize the surface
    pub fn resize(&mut self, size: (u32, u32)) {
        if size.0 == 0 || size.1 == 0 {
            return;
        }
        self.size = size;
        self.config.width = size.0;
        self.config.height = size.1;
        self.surface.configure(&self.device, &self.config);
    }
}

/// Request an adapter (On wasm we must use `request_adapter`)
#[cfg(target_arch = "wasm32")]
async fn request_adapter<'a>(
    instance: &wgpu::Instance,
    surface: &wgpu::Surface<'a>,
) -> anyhow::Result<wgpu::Adapter> {
    let adapter = instance
        .request_adapter(&wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::default(),
            compatible_surface: Some(surface),
            force_fallback_adapter: false,
        })
        .await
        .ok_or_else(|| anyhow::anyhow!("Failed to find an adapter"))?;
    Ok(adapter)
}

// On non-wasm platforms we can use `enumerate_adapters`, which offers more device support
#[cfg(not(target_arch = "wasm32"))]
async fn request_adapter<'a>(
    instance: &wgpu::Instance,
    surface: &wgpu::Surface<'a>,
) -> anyhow::Result<wgpu::Adapter> {
    let adapter = instance
        .enumerate_adapters(wgpu::Backends::all())
        .into_iter()
        .filter(|adapter| {
            // Check if this adapter supports our surface
            adapter.is_surface_supported(&surface)
        })
        .next()
        .ok_or(anyhow::anyhow!("Failed to find an adapter"))?;
    Ok(adapter)
}

/// Request the device and queue
async fn request_device(adapter: &wgpu::Adapter) -> anyhow::Result<(wgpu::Device, wgpu::Queue)> {
    let (device, queue) = adapter
        .request_device(
            &wgpu::DeviceDescriptor {
                required_features: wgpu::Features::empty(),
                required_limits: if cfg!(target_arch = "wasm32") {
                    // On wasm we must use the minimum limits
                    wgpu::Limits {
                        max_storage_textures_per_shader_stage: 1,
                        ..wgpu::Limits::downlevel_webgl2_defaults()
                    }
                } else {
                    wgpu::Limits::default()
                },
                label: None,
            },
            None,
        )
        .await
        .map_err(|e| anyhow::anyhow!("Failed to create device: {:?}", e))?;
    Ok((device, queue))
}

/// Configure the surface
fn configure_surface(
    adapter: &wgpu::Adapter,
    device: &wgpu::Device,
    surface: &wgpu::Surface,
    size: (u32, u32),
) -> wgpu::SurfaceConfiguration {
    let surface_capabilities = surface.get_capabilities(adapter);
    let surface_format = surface_capabilities
        .formats
        .iter()
        .copied()
        .filter(|format| format.is_srgb())
        .next()
        .unwrap_or(surface_capabilities.formats.first().unwrap().clone());

    let present_mode = surface_capabilities
        .present_modes
        .iter()
        .copied()
        .find(|mode| *mode == wgpu::PresentMode::Mailbox)
        .unwrap_or(wgpu::PresentMode::Fifo);
    let alpha_mode = surface_capabilities
        .alpha_modes
        .iter()
        .copied()
        .find(|mode| *mode == wgpu::CompositeAlphaMode::PreMultiplied)
        .unwrap_or(wgpu::CompositeAlphaMode::Opaque);

    let config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: surface_format,
        width: size.0,
        height: size.1,
        present_mode,
        alpha_mode,
        view_formats: vec![],
        desired_maximum_frame_latency: 1,
    };
    surface.configure(&device, &config);
    config
}
