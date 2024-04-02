use crate::color::Color;

use super::WgpuState;

/// The frame buffer shader
const FRAMEBUFFER_SHADER: &str = include_str!("../shaders/framebuffer.wgsl");

/// The frame buffer handles displaying the render output to a fullscreen surface
pub struct FrameBuffer {
    texture: wgpu::Texture,
    texture_view: wgpu::TextureView,
    sampler: wgpu::Sampler,
    render_pipeline: wgpu::RenderPipeline,
    bind_group: wgpu::BindGroup,
}

impl FrameBuffer {
    /// Create a new frame buffer
    pub fn new(wgpu_state: &WgpuState, resolution: (u32, u32)) -> Self {
        let (texture, texture_view) = create_texture(&wgpu_state.device, resolution);
        let sampler = create_sampler(&wgpu_state.device);

        let shader = compile_shader(&wgpu_state.device, FRAMEBUFFER_SHADER);

        let bind_group_layout = create_bind_group_layout(&wgpu_state.device);
        let bind_group = create_bind_group(
            &wgpu_state.device,
            &bind_group_layout,
            &texture_view,
            &sampler,
        );

        let pipeline_layout = create_pipeline_layout(&wgpu_state.device, &bind_group_layout);
        let render_pipeline = create_render_pipeline(
            &wgpu_state.device,
            wgpu_state.config.format,
            &shader,
            &pipeline_layout,
        );

        Self {
            texture,
            texture_view,
            sampler,
            bind_group,
            render_pipeline,
        }
    }

    /// Draw the frame buffer
    pub fn render(&mut self, wgpu_state: &mut WgpuState, clear_color: Color) -> anyhow::Result<()> {
        let output = match wgpu_state.surface.get_current_texture() {
            Ok(output) => output,
            Err(e) => {
                match e {
                    wgpu::SurfaceError::Outdated => {
                        wgpu_state.resize(wgpu_state.size);
                    }
                    wgpu::SurfaceError::Lost => {
                        log::error!("Surface lost");
                        return Err(anyhow::anyhow!("Surface lost"));
                    }
                    wgpu::SurfaceError::OutOfMemory => {
                        log::error!("Out of memory");
                        return Err(anyhow::anyhow!("Out of memory"));
                    }
                    _ => {}
                }
                return Ok(());
            }
        };
        let view = output
            .texture
            .create_view(&wgpu::TextureViewDescriptor::default());

        let mut encoder =
            wgpu_state
                .device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("FrameBuffer Encoder"),
                });

        {
            let color = clear_color.to_linear();
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("FrameBuffer Render Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: color.0 as f64,
                            g: color.1 as f64,
                            b: color.2 as f64,
                            a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                occlusion_query_set: None,
                timestamp_writes: None,
            });

            render_pass.set_pipeline(&self.render_pipeline);
            render_pass.set_bind_group(0, &self.bind_group, &[]);
            render_pass.draw(0..3, 0..1);
        }
        wgpu_state.queue.submit(std::iter::once(encoder.finish()));
        output.present();
        Ok(())
    }

    /// Get the texture view
    pub fn texture(&self) -> &wgpu::Texture {
        &self.texture
    }

    /// Get the texture view
    pub fn texture_view(&self) -> &wgpu::TextureView {
        &self.texture_view
    }

    /// Get the sampler
    pub fn sampler(&self) -> &wgpu::Sampler {
        &self.sampler
    }
}

/// Create the render texture
fn create_texture(device: &wgpu::Device, size: (u32, u32)) -> (wgpu::Texture, wgpu::TextureView) {
    let texture = device.create_texture(&wgpu::TextureDescriptor {
        label: Some("FrameBuffer Texture"),
        size: wgpu::Extent3d {
            width: size.0,
            height: size.1,
            depth_or_array_layers: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba32Float,
        usage: wgpu::TextureUsages::COPY_DST
            | wgpu::TextureUsages::TEXTURE_BINDING
            | wgpu::TextureUsages::STORAGE_BINDING,
        view_formats: &[wgpu::TextureFormat::Rgba32Float],
    });

    let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
    (texture, view)
}

/// Create the sampler
fn create_sampler(device: &wgpu::Device) -> wgpu::Sampler {
    device.create_sampler(&wgpu::SamplerDescriptor {
        label: Some("FrameBuffer Sampler"),
        address_mode_u: wgpu::AddressMode::ClampToEdge,
        address_mode_v: wgpu::AddressMode::ClampToEdge,
        address_mode_w: wgpu::AddressMode::ClampToEdge,
        mag_filter: wgpu::FilterMode::Nearest,
        min_filter: wgpu::FilterMode::Nearest,
        mipmap_filter: wgpu::FilterMode::Nearest,
        lod_min_clamp: 0.0,
        lod_max_clamp: 100.0,
        compare: None,
        anisotropy_clamp: 1,
        border_color: None,
    })
}

/// Compile the shader module
fn compile_shader(device: &wgpu::Device, shader: &str) -> wgpu::ShaderModule {
    device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("FrameBuffer Shader"),
        source: wgpu::ShaderSource::Wgsl(shader.into()),
    })
}

/// Create the render pipeline
fn create_render_pipeline(
    device: &wgpu::Device,
    texture_format: wgpu::TextureFormat,
    shader: &wgpu::ShaderModule,
    pipeline_layout: &wgpu::PipelineLayout,
) -> wgpu::RenderPipeline {
    device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
        label: Some("FrameBuffer Pipeline"),
        layout: Some(&pipeline_layout),
        vertex: wgpu::VertexState {
            module: shader,
            entry_point: "vertex",
            buffers: &[],
        },
        fragment: Some(wgpu::FragmentState {
            module: shader,
            entry_point: "fragment",
            targets: &[Some(wgpu::ColorTargetState {
                format: texture_format,
                blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                write_mask: wgpu::ColorWrites::ALL,
            })],
        }),
        primitive: wgpu::PrimitiveState {
            topology: wgpu::PrimitiveTopology::TriangleList,
            strip_index_format: None,
            front_face: wgpu::FrontFace::Ccw,
            cull_mode: None,
            polygon_mode: wgpu::PolygonMode::Fill,
            unclipped_depth: false,
            conservative: false,
        },
        depth_stencil: None,
        multisample: wgpu::MultisampleState::default(),
        multiview: None,
    })
}

/// Create the pipeline layout
fn create_pipeline_layout(
    device: &wgpu::Device,
    bind_group_layout: &wgpu::BindGroupLayout,
) -> wgpu::PipelineLayout {
    device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
        label: Some("FrameBuffer Pipeline Layout"),
        bind_group_layouts: &[&bind_group_layout],
        push_constant_ranges: &[],
    })
}

/// Create the bind group layout
fn create_bind_group_layout(device: &wgpu::Device) -> wgpu::BindGroupLayout {
    device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("FrameBuffer Bind Group Layout"),
        entries: &[
            wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Texture {
                    multisampled: false,
                    sample_type: wgpu::TextureSampleType::Float { filterable: false },
                    view_dimension: wgpu::TextureViewDimension::D2,
                },
                count: None,
            },
            wgpu::BindGroupLayoutEntry {
                binding: 1,
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::NonFiltering),
                count: None,
            },
        ],
    })
}

/// Create the bind group
fn create_bind_group(
    device: &wgpu::Device,
    layout: &wgpu::BindGroupLayout,
    texture_view: &wgpu::TextureView,
    sampler: &wgpu::Sampler,
) -> wgpu::BindGroup {
    device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("FrameBuffer Bind Group"),
        layout,
        entries: &[
            wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::TextureView(texture_view),
            },
            wgpu::BindGroupEntry {
                binding: 1,
                resource: wgpu::BindingResource::Sampler(sampler),
            },
        ],
    })
}
