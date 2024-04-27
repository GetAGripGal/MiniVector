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
    pub screen_size: glam::Vec2,
}

/// The bind groups for the electron gun simulation
#[derive(Debug)]
pub struct ElectronBindGroups {
    pub texture: wgpu::BindGroup,
    pub color: wgpu::BindGroup,
    pub points: wgpu::BindGroup,
    pub params: wgpu::BindGroup,
}

/// The bind group layouts for the electron gun simulation
#[derive(Debug)]
pub struct ElectronBindGroupLayouts {
    pub texture: wgpu::BindGroupLayout,
    pub color: wgpu::BindGroupLayout,
    pub points: wgpu::BindGroupLayout,
    pub params: wgpu::BindGroupLayout,
}

/// The buffers for the electron gun simulation
#[derive(Debug)]
pub struct ElectronBuffers {
    pub color: wgpu::Buffer,
    pub points: wgpu::Buffer,
    pub point_amount: wgpu::Buffer,
    pub params: wgpu::Buffer,
}

/// Renders the electron gun simulation using the compute shader
#[derive(Debug)]
pub struct ElectronRenderer {
    compute_pipeline: wgpu::ComputePipeline,

    texture: wgpu::Texture,
    texture_view: wgpu::TextureView,

    buffers: ElectronBuffers,
    bind_groups: ElectronBindGroups,
    layouts: ElectronBindGroupLayouts,
}

impl ElectronRenderer {
    /// Create a new electron renderer
    pub fn new(
        wgpu_state: &WgpuState,
        framebuffer: &FrameBuffer,
        point_buffer_size: usize,
        params: &ElectronParams,
        resolution: (u32, u32),
    ) -> anyhow::Result<Self> {
        let module = create_shader_module(&wgpu_state.device, ELECTRON_COMPUTE);

        let buffers = create_buffers(&wgpu_state.device, point_buffer_size, params);
        let layouts = create_bind_group_layouts(&wgpu_state.device);

        let (texture, texture_view) = create_texture(
            &wgpu_state.device,
            wgpu::Extent3d {
                width: resolution.0,
                height: resolution.1,
                depth_or_array_layers: 1,
            },
        );
        let bind_groups = create_bind_groups(
            &wgpu_state.device,
            &texture_view,
            framebuffer,
            &buffers,
            &layouts,
        );

        let compute_pipeline = create_compute_pipeline(&wgpu_state.device, &module, &layouts);

        Ok(Self {
            compute_pipeline,
            texture,
            texture_view,
            buffers,
            bind_groups,
            layouts,
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
            cpass.set_bind_group(0, &self.bind_groups.texture, &[]);
            cpass.set_bind_group(1, &self.bind_groups.color, &[]);
            cpass.set_bind_group(2, &self.bind_groups.points, &[]);
            cpass.set_bind_group(3, &self.bind_groups.params, &[]);
            cpass.dispatch_workgroups(compute_workgroup_count(x), compute_workgroup_count(y), 1);
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

    /// Clear the texture by filling it with zeros
    pub fn clear(&self, wgpu_state: &WgpuState) {
        let mut encoder =
            wgpu_state
                .device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Electron Command Encoder"),
                });

        let size = self.texture.size();
        let contents = vec![0u8; 4 * size.width as usize * size.height as usize];
        let buffer = wgpu_state
            .device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("Clear Buffer"),
                contents: &contents,
                usage: wgpu::BufferUsages::COPY_SRC,
            });

        encoder.copy_buffer_to_texture(
            wgpu::ImageCopyBuffer {
                buffer: &buffer,
                layout: wgpu::ImageDataLayout {
                    offset: 0,
                    bytes_per_row: None,
                    rows_per_image: None,
                },
            },
            wgpu::ImageCopyTexture {
                texture: &self.texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            size,
        );

        wgpu_state.queue.submit(std::iter::once(encoder.finish()));
    }

    /// Set the points
    pub fn set_points(&mut self, wgpu_state: &WgpuState, points: &VecDeque<Point>) -> Result<()> {
        let mut buffer = StorageBuffer::new(vec![]);
        buffer.write(points).map_err(|e| anyhow::anyhow!(e))?;
        // Update the point buffer size
        self.update_point_buffer_size(wgpu_state, points.len());
        // Write the buffer
        wgpu_state
            .queue
            .write_buffer(&self.buffers.points, 0, &buffer.into_inner());

        // Set the point amount
        let amount = points.len() as u32;
        wgpu_state.queue.write_buffer(
            &self.buffers.point_amount,
            0,
            bytemuck::cast_slice(&[amount]),
        );

        Ok(())
    }

    /// Update the point buffer size
    fn update_point_buffer_size(&mut self, wgpu_state: &WgpuState, size: usize) {
        let aligned_point_size = wgpu::util::align_to(std::mem::size_of::<Point>() as u64, 16);
        let new_size = aligned_point_size * size as u64;
        if new_size != self.buffers.points.size() && new_size > 0 {
            self.buffers.points = wgpu_state.device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("Point Buffer"),
                size: new_size,
                usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            });
            self.bind_groups.points = create_point_bind_group(
                &wgpu_state.device,
                &self.layouts.points,
                &self.buffers.points,
                &self.buffers.point_amount,
            );
        }
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
            .write_buffer(&self.buffers.color, 0, &buffer.into_inner());
        Ok(())
    }

    /// Update the electron params
    pub fn update_params(&mut self, wgpu_state: &WgpuState, params: &ElectronParams) -> Result<()> {
        let mut buffer = UniformBuffer::new(vec![]);
        buffer.write(params).map_err(|e| anyhow::anyhow!(e))?;
        wgpu_state
            .queue
            .write_buffer(&self.buffers.params, 0, &buffer.into_inner());
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

/// Create the buffers
fn create_buffers(
    device: &wgpu::Device,
    point_buffer_size: usize,
    params: &ElectronParams,
) -> ElectronBuffers {
    let color = create_color_buffer(device);
    let points = create_point_buffer(device, point_buffer_size);
    let point_amount = create_point_amount_buffer(device);
    let params = create_param_buffer(device, params);
    ElectronBuffers {
        color,
        points,
        point_amount,
        params,
    }
}

/// Create the bind group layouts
fn create_bind_group_layouts(device: &wgpu::Device) -> ElectronBindGroupLayouts {
    let texture = create_texture_bind_group_layout(device);
    let color = create_color_buffer_bind_group_layout(device);
    let points = create_point_bind_group_layout(device);
    let params = create_param_bind_group_layout(device);
    ElectronBindGroupLayouts {
        texture,
        color,
        points,
        params,
    }
}

/// Create the bind groups
fn create_bind_groups(
    device: &wgpu::Device,
    texture_view: &wgpu::TextureView,
    framebuffer: &FrameBuffer,
    buffers: &ElectronBuffers,
    layouts: &ElectronBindGroupLayouts,
) -> ElectronBindGroups {
    let texture = create_texture_bind_group(device, &layouts.texture, texture_view, framebuffer);
    let color = create_color_buffer_bind_group(device, &layouts.color, &buffers.color);
    let points = create_point_bind_group(
        device,
        &layouts.points,
        &buffers.points,
        &buffers.point_amount,
    );
    let params = create_param_bind_group(device, &layouts.params, &buffers.params);
    ElectronBindGroups {
        texture,
        color,
        points,
        params,
    }
}

/// Create a compute pipeline and bind group
fn create_compute_pipeline(
    device: &wgpu::Device,
    module: &wgpu::ShaderModule,
    layouts: &ElectronBindGroupLayouts,
) -> wgpu::ComputePipeline {
    let pipeline_layout = create_pipeline_layout(
        device,
        &layouts.texture,
        &layouts.color,
        &layouts.points,
        &layouts.params,
    );
    let pipeline = device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
        label: Some("Electron Compute Pipeline"),
        layout: Some(&pipeline_layout),
        module,
        entry_point: "main",
    });

    pipeline
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
        usage: wgpu::TextureUsages::STORAGE_BINDING
            | wgpu::TextureUsages::COPY_SRC
            | wgpu::TextureUsages::COPY_DST,
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

/// Figure out how many workgroups we need to dispatch based on the size of the texture
fn compute_workgroup_count(size: u32) -> u32 {
    (size + COMPUTE_WORKGROUP_SIZE - 1) / COMPUTE_WORKGROUP_SIZE
}
