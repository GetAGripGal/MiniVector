use std::collections::VecDeque;

use encase::{ShaderType, StorageBuffer, UniformBuffer};
use wgpu::util::DeviceExt;

use super::{display_colors::DisplayColors, framebuffer::FrameBuffer, point::Point, WgpuState};
use crate::{color::Color, result::Result};

/// The workgroup size for the compute shader
const COMPUTE_WORKGROUP_SIZE: u32 = 8;

/// The compute shader for the electron gun simulation
const ELECTRON_COMPUTE: &'static str = include_str!("../shaders/electron.wgsl");

/// The parameters for the electron gun simulation
#[derive(Debug, ShaderType, Clone)]
pub struct ElectronParams {
    pub radius: f32,
    pub dim_factor: f32,
}

/// Renders the electron gun simulation using the compute shader
#[derive(Debug)]
pub struct ElectronRenderer {
    compute_pipeline: wgpu::ComputePipeline,
    texture_bind_group: wgpu::BindGroup,

    texture: wgpu::Texture,
    texture_view: wgpu::TextureView,

    color_bind_group: wgpu::BindGroup,
    color_buffer: wgpu::Buffer,

    point_bind_group: wgpu::BindGroup,
    point_buffer: wgpu::Buffer,
    point_amount_buffer: wgpu::Buffer,

    param_buffer: wgpu::Buffer,
    param_bind_group: wgpu::BindGroup,
}

impl ElectronRenderer {
    /// Create a new electron renderer
    pub fn new(
        wgpu_state: &WgpuState,
        framebuffer: &FrameBuffer,
        point_buffer_size: usize,
        params: &ElectronParams,
    ) -> anyhow::Result<Self> {
        let module = create_shader_module(&wgpu_state.device, ELECTRON_COMPUTE);
        let color_buffer = create_color_buffer(&wgpu_state.device);
        let point_buffer = create_point_buffer(&wgpu_state.device, point_buffer_size);
        let point_amount_buffer = create_point_amount_buffer(&wgpu_state.device);
        let param_buffer = create_param_buffer(&wgpu_state.device, params);

        let (texture, texture_view) =
            create_texture(&wgpu_state.device, framebuffer.texture().size());

        let (
            compute_pipeline,
            texture_bind_group,
            color_bind_group,
            point_bind_group,
            param_bind_group,
        ) = create_compute_pipeline(
            &wgpu_state.device,
            &module,
            &color_buffer,
            &point_buffer,
            &point_amount_buffer,
            &param_buffer,
            &texture_view,
            framebuffer,
        );

        Ok(Self {
            compute_pipeline,
            texture_bind_group,
            texture,
            texture_view,
            color_bind_group,
            color_buffer,
            point_bind_group,
            point_buffer,
            point_amount_buffer,
            param_buffer,
            param_bind_group,
        })
    }

    /// Process the electron gun simulation
    pub fn process(&self, wgpu_state: &mut WgpuState, framebuffer: &FrameBuffer) {
        let mut encoder =
            wgpu_state
                .device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Electron Command Encoder"),
                });

        {
            let x = framebuffer.texture().size().width;
            let y = framebuffer.texture().size().height;
            let mut cpass = encoder.begin_compute_pass(&Default::default());
            cpass.set_pipeline(&self.compute_pipeline);
            cpass.set_bind_group(0, &self.texture_bind_group, &[]);
            cpass.set_bind_group(1, &self.color_bind_group, &[]);
            cpass.set_bind_group(2, &self.point_bind_group, &[]);
            cpass.set_bind_group(3, &self.param_bind_group, &[]);
            cpass.dispatch_workgroups(x / COMPUTE_WORKGROUP_SIZE, y / COMPUTE_WORKGROUP_SIZE, 1);
        }

        // Copy the texture to the framebuffer
        encoder.copy_texture_to_texture(
            self.texture.as_image_copy(),
            framebuffer.texture().as_image_copy(),
            wgpu::Extent3d {
                width: framebuffer.texture().size().width,
                height: framebuffer.texture().size().height,
                depth_or_array_layers: 1,
            },
        );

        wgpu_state.queue.submit(std::iter::once(encoder.finish()));
    }

    /// Set the points
    pub fn set_points(&mut self, wgpu_state: &WgpuState, points: &VecDeque<Point>) -> Result<()> {
        let mut buffer = StorageBuffer::new(vec![]);
        buffer.write(points).map_err(|e| anyhow::anyhow!(e))?;
        wgpu_state
            .queue
            .write_buffer(&self.point_buffer, 0, &buffer.into_inner());
        wgpu_state.queue.write_buffer(
            &self.point_amount_buffer,
            0,
            bytemuck::cast_slice(&[points.len() as u32]),
        );
        Ok(())
    }

    /// Set the display colors
    pub fn set_display_colors(
        &mut self,
        wgpu_state: &WgpuState,
        primary: Color,
        secondary: Color,
    ) -> Result<()> {
        let colors = DisplayColors {
            primary: primary.into(),
            secondary: secondary.into(),
        };
        let mut buffer = StorageBuffer::new(vec![]);
        buffer.write(&colors).map_err(|e| anyhow::anyhow!(e))?;
        wgpu_state
            .queue
            .write_buffer(&self.color_buffer, 0, &buffer.into_inner());
        Ok(())
    }

    /// Update the electron params
    pub fn update_params(&mut self, wgpu_state: &WgpuState, params: &ElectronParams) -> Result<()> {
        let mut buffer = UniformBuffer::new(vec![]);
        buffer.write(params).map_err(|e| anyhow::anyhow!(e))?;
        wgpu_state
            .queue
            .write_buffer(&self.param_buffer, 0, &buffer.into_inner());
        Ok(())
    }

    /// Get the texture
    pub fn texture(&self) -> &wgpu::Texture {
        &self.texture
    }

    /// Get the texture view
    pub fn texture_view(&self) -> &wgpu::TextureView {
        &self.texture_view
    }
}

/// Create a shader module from the source
fn create_shader_module(device: &wgpu::Device, source: &str) -> wgpu::ShaderModule {
    device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("Electron Shader Module"),
        source: wgpu::ShaderSource::Wgsl(source.into()),
    })
}

/// Create a compute pipeline and bind group
fn create_compute_pipeline(
    device: &wgpu::Device,
    module: &wgpu::ShaderModule,
    color_buffer: &wgpu::Buffer,
    point_buffer: &wgpu::Buffer,
    point_amount_buffer: &wgpu::Buffer,
    param_buffer: &wgpu::Buffer,
    texture_view: &wgpu::TextureView,
    framebuffer: &FrameBuffer,
) -> (
    wgpu::ComputePipeline,
    wgpu::BindGroup,
    wgpu::BindGroup,
    wgpu::BindGroup,
    wgpu::BindGroup,
) {
    let texture_bind_group_layout = create_texture_bind_group_layout(device);
    let texture_bind_group = create_texture_bind_group(
        device,
        &texture_bind_group_layout,
        texture_view,
        framebuffer,
    );

    let color_bind_group_layout = create_color_buffer_bind_group_layout(device);
    let color_bind_group =
        create_color_buffer_bind_group(device, &color_bind_group_layout, color_buffer);

    let point_bind_group_layout = create_point_bind_group_layout(device);
    let point_bind_group = create_point_bind_group(
        device,
        &point_bind_group_layout,
        &point_buffer,
        &point_amount_buffer,
    );

    let param_bind_group_layout = create_param_bind_group_layout(device);
    let param_bind_group = create_param_bind_group(device, &param_bind_group_layout, param_buffer);

    let pipeline_layout = create_pipeline_layout(
        device,
        &texture_bind_group_layout,
        &color_bind_group_layout,
        &point_bind_group_layout,
        &param_bind_group_layout,
    );
    let pipeline = device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
        label: Some("Electron Compute Pipeline"),
        layout: Some(&pipeline_layout),
        module,
        entry_point: "main",
    });

    (
        pipeline,
        texture_bind_group,
        color_bind_group,
        point_bind_group,
        param_bind_group,
    )
}

/// Create the bind group layout
fn create_texture_bind_group_layout(device: &wgpu::Device) -> wgpu::BindGroupLayout {
    device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("Electron Bind Group Layout"),
        entries: &[
            // The storage texture
            wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::StorageTexture {
                    access: wgpu::StorageTextureAccess::WriteOnly,
                    format: wgpu::TextureFormat::Rgba32Float,
                    view_dimension: wgpu::TextureViewDimension::D2,
                },
                count: None,
            },
            // The input texture
            wgpu::BindGroupLayoutEntry {
                binding: 1,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Texture {
                    sample_type: wgpu::TextureSampleType::Float { filterable: false },
                    view_dimension: wgpu::TextureViewDimension::D2,
                    multisampled: false,
                },
                count: None,
            },
            // The sampler
            wgpu::BindGroupLayoutEntry {
                binding: 2,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                count: None,
            },
        ],
    })
}

/// Create the pipeline layout
fn create_pipeline_layout(
    device: &wgpu::Device,
    texture_bind_group_layout: &wgpu::BindGroupLayout,
    color_bind_group_layout: &wgpu::BindGroupLayout,
    point_bind_group_layout: &wgpu::BindGroupLayout,
    params_bind_group_layout: &wgpu::BindGroupLayout,
) -> wgpu::PipelineLayout {
    device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
        label: Some("Electron Pipeline Layout"),
        bind_group_layouts: &[
            texture_bind_group_layout,
            color_bind_group_layout,
            point_bind_group_layout,
            params_bind_group_layout,
        ],
        push_constant_ranges: &[],
    })
}

/// Create the texture bind group
fn create_texture_bind_group(
    device: &wgpu::Device,
    bind_group_layout: &wgpu::BindGroupLayout,
    texture_view: &wgpu::TextureView,
    framebuffer: &FrameBuffer,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("Electron Bind Group"),
        layout: bind_group_layout,
        entries: &[
            // The storage texture
            wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::TextureView(texture_view),
            },
            // The input texture
            wgpu::BindGroupEntry {
                binding: 1,
                resource: wgpu::BindingResource::TextureView(framebuffer.texture_view()),
            },
            // The sampler
            wgpu::BindGroupEntry {
                binding: 2,
                resource: wgpu::BindingResource::Sampler(framebuffer.sampler()),
            },
        ],
    })
}

/// Create a texture for the electron gun simulation
fn create_texture(
    device: &wgpu::Device,
    size: wgpu::Extent3d,
) -> (wgpu::Texture, wgpu::TextureView) {
    let texture = device.create_texture(&wgpu::TextureDescriptor {
        label: Some("Electron Texture"),
        size,
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba32Float,
        usage: wgpu::TextureUsages::STORAGE_BINDING | wgpu::TextureUsages::COPY_SRC,
        view_formats: &[wgpu::TextureFormat::Rgba32Float],
    });
    let view = texture.create_view(&Default::default());
    (texture, view)
}

/// Create the color buffer bind group layout
fn create_color_buffer_bind_group_layout(device: &wgpu::Device) -> wgpu::BindGroupLayout {
    device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("Color Buffer Bind Group Layout"),
        entries: &[wgpu::BindGroupLayoutEntry {
            binding: 0,
            visibility: wgpu::ShaderStages::COMPUTE,
            ty: wgpu::BindingType::Buffer {
                ty: wgpu::BufferBindingType::Storage { read_only: true },
                has_dynamic_offset: false,
                min_binding_size: None,
            },
            count: None,
        }],
    })
}

/// Create the color buffer bind group
fn create_color_buffer_bind_group(
    device: &wgpu::Device,
    bind_group_layout: &wgpu::BindGroupLayout,
    color_buffer: &wgpu::Buffer,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("Color Buffer Bind Group"),
        layout: bind_group_layout,
        entries: &[wgpu::BindGroupEntry {
            binding: 0,
            resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                buffer: color_buffer,
                offset: 0,
                size: None,
            }),
        }],
    })
}

/// Create the color buffer
fn create_color_buffer(device: &wgpu::Device) -> wgpu::Buffer {
    let aligned_size = wgpu::util::align_to(std::mem::size_of::<DisplayColors>() as u64, 16);
    device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("Color Buffer"),
        size: aligned_size,
        usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_DST,
        mapped_at_creation: false,
    })
}

/// Create the point buffer
fn create_point_buffer(device: &wgpu::Device, point_buffer_size: usize) -> wgpu::Buffer {
    let aligned_point_size = wgpu::util::align_to(std::mem::size_of::<Point>() as u64, 16);
    device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("Point Buffer"),
        size: aligned_point_size * point_buffer_size as u64,
        usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_DST,
        mapped_at_creation: false,
    })
}

/// Create the point amount buffer
fn create_point_amount_buffer(device: &wgpu::Device) -> wgpu::Buffer {
    device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("Point Amount Buffer"),
        size: std::mem::size_of::<u32>() as u64,
        usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_DST,
        mapped_at_creation: false,
    })
}

/// Create the point bind group layout
fn create_point_bind_group_layout(device: &wgpu::Device) -> wgpu::BindGroupLayout {
    device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("Point Bind Group Layout"),
        entries: &[
            wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: true },
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            },
            wgpu::BindGroupLayoutEntry {
                binding: 1,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: true },
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            },
        ],
    })
}

/// Create the point bind group
fn create_point_bind_group(
    device: &wgpu::Device,
    bind_group_layout: &wgpu::BindGroupLayout,
    point_buffer: &wgpu::Buffer,
    point_amount_buffer: &wgpu::Buffer,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("Point Bind Group"),
        layout: bind_group_layout,
        entries: &[
            wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                    buffer: point_buffer,
                    offset: 0,
                    size: None,
                }),
            },
            wgpu::BindGroupEntry {
                binding: 1,
                resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                    buffer: point_amount_buffer,
                    offset: 0,
                    size: None,
                }),
            },
        ],
    })
}

/// Create the electron params buffer
fn create_param_buffer(device: &wgpu::Device, params: &ElectronParams) -> wgpu::Buffer {
    let mut buffer = UniformBuffer::new(vec![]);
    buffer.write(params).unwrap();
    device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: Some("Electron Params Buffer"),
        contents: &buffer.into_inner(),
        usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
    })
}

/// Create the electron params bind group layout
fn create_param_bind_group_layout(device: &wgpu::Device) -> wgpu::BindGroupLayout {
    device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("Electron Params Bind Group Layout"),
        entries: &[wgpu::BindGroupLayoutEntry {
            binding: 0,
            visibility: wgpu::ShaderStages::COMPUTE,
            ty: wgpu::BindingType::Buffer {
                ty: wgpu::BufferBindingType::Uniform,
                has_dynamic_offset: false,
                min_binding_size: None,
            },
            count: None,
        }],
    })
}

/// Create the electron params bind group
fn create_param_bind_group(
    device: &wgpu::Device,
    bind_group_layout: &wgpu::BindGroupLayout,
    param_buffer: &wgpu::Buffer,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("Electron Params Bind Group"),
        layout: bind_group_layout,
        entries: &[wgpu::BindGroupEntry {
            binding: 0,
            resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                buffer: param_buffer,
                offset: 0,
                size: None,
            }),
        }],
    })
}
