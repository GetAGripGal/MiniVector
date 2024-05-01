use encase::{ShaderType, StorageBuffer, UniformBuffer};
use wgpu::util::DeviceExt;

use super::{
    display_colors::DisplayColors,
    framebuffer::FrameBuffer,
    point::{self, Point},
    WgpuState,
};
use crate::{color::Color, result::Result};

/// The workgroup size for the compute shader
const COMPUTE_WORKGROUP_SIZE: u32 = 8;

/// The compute shader for the electron gun simulation
const ELECTRON_COMPUTE: &'static str = include_str!("../shaders/electron.wgsl");

/// The parameters for the electron gun simulation
#[derive(Debug, ShaderType, Clone)]
pub struct ElectronParams {
    pub current_point: glam::Vec2,
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
    pub point_amount: wgpu::Buffer,
    pub params: wgpu::Buffer,
}

/// Renders the electron gun simulation using the compute shader
#[derive(Debug)]
pub struct ElectronRenderer {
    compute_pipeline: wgpu::ComputePipeline,

    texture: wgpu::Texture,
    texture_view: wgpu::TextureView,
    point_texture: wgpu::Texture,
    point_texture_view: wgpu::TextureView,

    buffers: ElectronBuffers,
    bind_groups: ElectronBindGroups,
    layouts: ElectronBindGroupLayouts,

    params: ElectronParams,
}

impl ElectronRenderer {
    /// Create a new electron renderer
    pub fn new(
        wgpu_state: &WgpuState,
        framebuffer: &FrameBuffer,
        params: &ElectronParams,
        resolution: (u32, u32),
    ) -> anyhow::Result<Self> {
        let module = create_shader_module(&wgpu_state.device, ELECTRON_COMPUTE);

        let buffers = create_buffers(&wgpu_state.device, params);
        let layouts = create_bind_group_layouts(&wgpu_state.device);

        let (texture, texture_view) = create_texture(
            &wgpu_state.device,
            wgpu::Extent3d {
                width: resolution.0,
                height: resolution.1,
                depth_or_array_layers: 1,
            },
        );
        let (point_texture, point_texture_view) = create_point_lookup_texture(
            &wgpu_state.device,
            &wgpu_state.queue,
            &Vec::new(),
            params.screen_size.into(),
        );

        let bind_groups = create_bind_groups(
            &wgpu_state.device,
            &texture_view,
            &point_texture_view,
            framebuffer,
            &buffers,
            &layouts,
        );

        let compute_pipeline = create_compute_pipeline(&wgpu_state.device, &module, &layouts);

        Ok(Self {
            compute_pipeline,
            texture,
            texture_view,
            point_texture,
            point_texture_view,
            buffers,
            bind_groups,
            layouts,
            params: params.clone(),
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
    pub fn set_points(&mut self, wgpu_state: &WgpuState, points: &[Point]) -> Result<()> {
        // Update the point lookup texture
        (self.point_texture, self.point_texture_view) = create_point_lookup_texture(
            &wgpu_state.device,
            &wgpu_state.queue,
            points,
            self.params.screen_size.into(),
        );
        self.bind_groups.points = create_point_bind_group(
            &wgpu_state.device,
            &self.layouts.points,
            &self.point_texture_view,
            &self.buffers.point_amount,
        );

        // Update the point amount buffer
        let point_amount = points.len() as u32;
        wgpu_state.queue.write_buffer(
            &self.buffers.point_amount,
            0,
            &bytemuck::cast_slice(&[point_amount]),
        );

        // Recreate the params buffer
        if let Some(point) = points.iter().last() {
            self.params.current_point = (point.x, point.y).into();
        }
        let params = self.params.clone();
        self.update_params(wgpu_state, &params)?;
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
        self.params = params.clone();
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
fn create_buffers(device: &wgpu::Device, params: &ElectronParams) -> ElectronBuffers {
    let color = create_color_buffer(device);
    let point_amount = create_point_amount_buffer(device);
    let params = create_param_buffer(device, params);
    ElectronBuffers {
        color,
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
    point_lookup: &wgpu::TextureView,
    framebuffer: &FrameBuffer,
    buffers: &ElectronBuffers,
    layouts: &ElectronBindGroupLayouts,
) -> ElectronBindGroups {
    let texture = create_texture_bind_group(device, &layouts.texture, texture_view, framebuffer);
    let color = create_color_buffer_bind_group(device, &layouts.color, &buffers.color);
    let points =
        create_point_bind_group(device, &layouts.points, point_lookup, &buffers.point_amount);
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
                ty: wgpu::BindingType::Texture {
                    sample_type: wgpu::TextureSampleType::Uint,
                    view_dimension: wgpu::TextureViewDimension::D1,
                    multisampled: false,
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
    point_lookup: &wgpu::TextureView,
    point_amount_buffer: &wgpu::Buffer,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("Point Bind Group"),
        layout: bind_group_layout,
        entries: &[
            wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::TextureView(point_lookup),
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

// /// Pack x and y into a single u32 (must be normalized)
// fn pack_floats(x: f32, y: f32, screen_size: (f32, f32)) -> u32 {
//     let x_normalized = (x / screen_size.0).clamp(0.0, 1.0);
//     let y_normalized = (y / screen_size.1).clamp(0.0, 1.0);

//     let x_scaled = (x_normalized * 0xFFFF as f32) as u32;
//     let y_scaled = (y_normalized * 0xFFFF as f32) as u32;

//     (x_scaled << 16) | (y_scaled & 0xFFFF)
// }

/// Pack x and y into a single u32 (with an additional reserved byte as flag)
fn pack_point(x: f32, y: f32, power: u32, screen_size: (f32, f32)) -> u32 {
    // Normalize and clamp the floats
    let x_normalized = (x / screen_size.0).clamp(0.0, 1.0);
    let y_normalized = (y / screen_size.1).clamp(0.0, 1.0);

    // Scale the normalized floats to fit into 14 bits
    let x_scaled = (x_normalized * 0x3FFF as f32) as u32; // 0x3FFF = 14 bits set to 1
    let y_scaled = (y_normalized * 0x3FFF as f32) as u32;

    // Pack the floats into a u32 with the flag byte at the end
    ((x_scaled << 18) | (y_scaled << 4)) | power
}

fn unpack_point(data: u32, screen_size: (f32, f32)) -> (f32, f32, u32) {
    // Extract x, y, and power from the packed u32
    let x_scaled = (data >> 18) & 0x3FFF; // Extracting 14 bits for x
    let y_scaled = (data >> 4) & 0x3FFF; // Extracting 14 bits for y
    let power = data & 0x1; // Extracting power

    // Scale x and y back to their original values
    let x = (x_scaled as f32 / 0x3FFF as f32) * screen_size.0;
    let y = (y_scaled as f32 / 0x3FFF as f32) * screen_size.1;

    (x, y, power)
}

/// Create a texture from the point buffer
pub fn create_point_lookup_texture(
    device: &wgpu::Device,
    queue: &wgpu::Queue,
    points: &[Point],
    screen_size: (f32, f32),
) -> (wgpu::Texture, wgpu::TextureView) {
    // Normalize the points to rgb values representing (x, y, power)
    let normalize = |point: Point| {
        let packed = pack_point(point.x, point.y, point.power, screen_size);
        packed
    };
    let mut data = Vec::with_capacity(points.len());
    data.extend(points.iter().map(|p| normalize(*p)));

    // If data is empty. Add a blank point to avoid creating a 0 size texture
    if data.len() <= 0 {
        data.push(0);
    }

    // If the data limit is reached, shrink the data buffer
    // NOTE(GetAGripGal): There should be a way to increase this by turning the texture into a D2 texture.
    // But for now this is easiest
    const MAX_SIZE: usize = 8192;
    if data.len() > MAX_SIZE {
        data = data[0..MAX_SIZE].to_vec();
        log::warn!("Maximum amount of points reached! Some points may be removed");
    }

    let texture = device.create_texture_with_data(
        &queue,
        &wgpu::TextureDescriptor {
            label: Some("Point lookup texture"),
            size: wgpu::Extent3d {
                width: data.len() as u32,
                height: 1,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D1,
            format: wgpu::TextureFormat::R32Uint,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[wgpu::TextureFormat::R32Uint],
        },
        wgpu::util::TextureDataOrder::MipMajor,
        &bytemuck::cast_slice(&data),
    );

    let texture_view = texture.create_view(&Default::default());
    (texture, texture_view)
}

/// Figure out how many workgroups we need to dispatch based on the size of the texture
fn compute_workgroup_count(size: u32) -> u32 {
    (size + COMPUTE_WORKGROUP_SIZE - 1) / COMPUTE_WORKGROUP_SIZE
}
