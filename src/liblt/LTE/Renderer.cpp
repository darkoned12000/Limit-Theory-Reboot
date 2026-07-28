#include "Renderer.h"
#include "Bound.h"
#include "Color.h"
#include "CubeMap.h"
#include "GL.h"
#include "Matrix.h"
#include "Mesh.h"
#include "ProgramLog.h"
#include "Shader.h"
#include "ShaderInstance.h"
#include "Stack.h"
#include "Texture2D.h"
#include "Transform.h"
#include "Tuple.h"
#include "VectorMap.h"
#include "Window.h"

const bool kAllow16BitIndices = true;
const bool kCameraSpaceRendering = true;
const size_t kMaxColorAttachments = 4;
const size_t kFramebufferCacheSize = 128;

namespace {
  AutoClass(Attachment,
    GL_Texture, texture,
    GL_TextureTarget::Enum, target,
    uint, guid)
    Attachment() = default;
  };

  struct Renderer {
    Matrix world;
    Matrix view;
    Matrix proj;
    Matrix wvp;
    Transform viewTransform;
    int callCount;
    int callCountLast;
    int polyCount;
    int polyCountLast;

    bool attribArrayEnabled[kAttribArrays];
    GL_Buffer indexBuffer;
    GL_Buffer vertexBuffer;

    Stack<Attachment> colorAttachment[kMaxColorAttachments];
    Stack<GL_Texture> depthAttachment;

    Stack<BlendMode::Enum> blendMode;
    Stack<CullMode::Enum> cullMode;
    Stack< Tuple3<bool, V2, V2> > scissor;
    Stack< V4T<int> > viewport;
    Vector<int> zBuffer;
    Vector<int> zWritable;
    VectorMap<size_t, GL_Framebuffer> fboCache;
    
    Renderer() :
      callCount(0),
      callCountLast(0),
      polyCount(0),
      polyCountLast(0),
      indexBuffer(GL_NullBuffer),
      vertexBuffer(GL_NullBuffer)
      {}
  } renderer;

  void Renderer_ResetGlobalState() {
    /* Force synchronization of GL state. */
    for (size_t i = 0; i < kAttribArrays; ++i) {
      renderer.attribArrayEnabled[i] = false;
      GL_DisableVertexAttribArray(i);
    }

    renderer.indexBuffer = GL_NullBuffer;
    renderer.vertexBuffer = GL_NullBuffer;

    GL_BindBuffer(GL_BufferTarget::Array, GL_NullBuffer);
    GL_BindBuffer(GL_BufferTarget::ElementArray, GL_NullBuffer);
    Shader_UseFixedFunction();

    GL_Enable(GL_Capability::CullFace);
    GL_Enable(GL_Capability::CubemapSeamless);
    GL_CullFace(GL_CullMode::Back);
    GL_FrontFace(GL_FaceOrientation::CCW);
    GL_Disable(GL_Capability::MultiSample);
  }

  void Renderer_FlushFBOCache() {
    if (renderer.fboCache.size() > kFramebufferCacheSize) {
      for (size_t i = 0; i < renderer.fboCache.size(); ++i)
        GL_DeleteFramebuffer(renderer.fboCache.entries[i].value);
      renderer.fboCache.clear();
    }
  }

  void Renderer_UpdateFramebuffer() {
    static std::vector<GLenum> buffers;
    buffers.clear();

    Hasher hasher;
    bool hasAttachment = false;
    for (size_t i = 0; i < kMaxColorAttachments; ++i) {
      if (renderer.colorAttachment[i].size() &&
          renderer.colorAttachment[i].back().texture != GL_NullTexture)
      {
        buffers.push_back((GLenum)(GL_COLOR_ATTACHMENT0 + i));
        hasher | i | renderer.colorAttachment[i].back();
        hasAttachment = true;
      }
    }

    if (renderer.depthAttachment.size() &&
        renderer.depthAttachment.back() != GL_NullTexture)
    {
      hasher | (size_t)renderer.depthAttachment.back();
      hasAttachment = true;
    }

    if (!hasAttachment) {
      GL_BindFramebuffer(GL_FramebufferTarget::Draw, GL_NullFramebuffer);
      return;
    }

    Renderer_FlushFBOCache();

    HashT hash = (HashT)hasher;
    GL_Framebuffer* pBuffer = renderer.fboCache.get(hash);

    if (pBuffer) {
      GL_BindFramebuffer(GL_FramebufferTarget::Draw, *pBuffer);
    } else {
      GL_Framebuffer buffer = GL_GenFramebuffer();
      renderer.fboCache[hash] = buffer;

      GL_BindFramebuffer(GL_FramebufferTarget::Draw, buffer);

      for (size_t i = 0; i < kMaxColorAttachments; ++i)
        if (renderer.colorAttachment[i].size())
          GL_FramebufferTexture2D(
            GL_FramebufferTarget::Draw,
            (GL_FramebufferAttachment::Enum)
            (GL_FramebufferAttachment::ColorAttachment0 + i),
            renderer.colorAttachment[i].back().target,
            renderer.colorAttachment[i].back().texture,
            0);

      if (renderer.depthAttachment.size())
        GL_FramebufferTexture2D(
          GL_FramebufferTarget::Draw,
          GL_FramebufferAttachment::DepthAttachment,
          GL_TextureTarget::T2D,
          renderer.depthAttachment.back(),
          0);
    }

    glDrawBuffers(buffers.size(), buffers.data());
    DEBUG_GL_ERRORS;
  }
}

// ----------------------------------------------------------------------------
// GPU Instancing Infrastructure
// ----------------------------------------------------------------------------

namespace {
  /* Shared SSBO for per-instance world matrices (binding point 0).
     A dummy 128-byte buffer is bound at init so non-instanced shaders
     never access an unbound SSBO. The real instance buffer replaces it
     during instanced draws. */
  const unsigned int kInstanceBindingPoint = 0;
  GL_Buffer gDummyInstanceBuffer = GL_NullBuffer;
  GL_Buffer gInstanceBuffer = GL_NullBuffer;
  size_t gInstanceBufferCapacity = 0;

  GL_Buffer GetDummyInstanceBuffer() {
    if (gDummyInstanceBuffer == GL_NullBuffer) {
      gDummyInstanceBuffer = GL_GenBuffer();
      /* Must be at least sizeof(mat4) * 2 to match the GLSL InstanceData
         struct { mat4 world; mat4 worldIT; }. GLSL ternaries don't guarantee
         short-circuit, so non-instanced shaders may still touch the SSBO. */
      float zero[32] = {};
      GL_BindBuffer(GL_BufferTarget::ShaderStorage, gDummyInstanceBuffer);
      GL_BufferData(GL_BufferTarget::ShaderStorage, sizeof(zero), zero,
                    GL_BufferUsage::StaticDraw);
      GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                        kInstanceBindingPoint, gDummyInstanceBuffer);
    }
    return gDummyInstanceBuffer;
  }

  GL_Buffer GetInstanceBuffer() {
    if (gInstanceBuffer == GL_NullBuffer)
      gInstanceBuffer = GL_GenBuffer();
    return gInstanceBuffer;
  }

  void SetInstancedUniform(int instanced) {
    ShaderT* shader = Shader_GetActive();
    if (shader) {
      int loc = shader->QueryUniformLocation("uInstanced");
      if (loc >= 0)
        shader->SetInt(loc, instanced);
    }
  }

  void SetParticleInstancedUniform(int instanced) {
    ShaderT* shader = Shader_GetActive();
    if (shader) {
      int loc = shader->QueryUniformLocation("uParticleInstanced");
      if (loc >= 0)
        shader->SetInt(loc, instanced);
    }
  }

  /* Particle SSBO at binding point 1 — compact per-instance data for
     GPU-instanced billboard particles. Separate from the model instancing
     SSBO at binding point 0. */
  const unsigned int kParticleBindingPoint = 1;
  GL_Buffer gParticleDummyBuffer = GL_NullBuffer;
  GL_Buffer gParticleInstanceBuffer = GL_NullBuffer;
  size_t gParticleInstanceBufferCapacity = 0;

  GL_Buffer GetParticleDummyBuffer() {
    if (gParticleDummyBuffer == GL_NullBuffer) {
      gParticleDummyBuffer = GL_GenBuffer();
      /* 2 x vec4 = 32 bytes, matches ParticleInstanceData layout. */
      float zero[8] = {};
      GL_BindBuffer(GL_BufferTarget::ShaderStorage, gParticleDummyBuffer);
      GL_BufferData(GL_BufferTarget::ShaderStorage, sizeof(zero), zero,
                    GL_BufferUsage::StaticDraw);
      GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                        kParticleBindingPoint, gParticleDummyBuffer);
    }
    return gParticleDummyBuffer;
  }

  GL_Buffer GetParticleInstanceBuffer() {
    if (gParticleInstanceBuffer == GL_NullBuffer)
      gParticleInstanceBuffer = GL_GenBuffer();
    return gParticleInstanceBuffer;
  }
}

namespace LTE {
  void Renderer_Initialize() {
    char const* version = (char const*)glGetString(GL_VERSION);
    Log_Message(Stringize() | "OpenGL Version " | version);

    /* For OS X + Intel drivers, we need to make use of (supported) 3+
       functions, even though the driver will claim not to support 3. */
    glewExperimental = GL_TRUE;

    GLenum err = glewInit();
    if (err != GLEW_OK)
      Log_Critical("GLEW failed to initialize");
    if (!GLEW_VERSION_2_1)
      Log_Critical("GLEW failed to support OpenGL 2.1");

    /* Assert every OpenGL extension function that we're going to use, just
       to be safe! May help find compatability errors on older cards. */
    #define CHECK(x) if (!x)                                                   \
      Log_Critical(String("GL failed to support ") + #x + ". Hardware incompatibility!");

    CHECK(glActiveTexture);
    CHECK(glAttachShader);
    CHECK(glBindAttribLocation);
    CHECK(glBindBuffer);
    CHECK(glBindFragDataLocation);
    CHECK(glBindFramebuffer);
    CHECK(glBindRenderbuffer);
    CHECK(glBufferData);
    CHECK(glCompileShader);
    CHECK(glDeleteBuffers);
    CHECK(glDeleteProgram);
    CHECK(glDeleteShader);
    CHECK(glDisableVertexAttribArray);
    CHECK(glEnableVertexAttribArray);
    CHECK(glFramebufferTexture2D);
    CHECK(glGenBuffers);
    CHECK(glGenRenderbuffers);
    CHECK(glGenerateMipmap);
    CHECK(glGenFramebuffers);
    CHECK(glGetProgramiv);
    CHECK(glGetProgramInfoLog);
    CHECK(glGetShaderInfoLog);
    CHECK(glGetUniformLocation);
    CHECK(glLinkProgram);
    CHECK(glShaderSource);
    CHECK(glTexImage3D);
    CHECK(glTexSubImage3D);
    CHECK(glUniform1f);
    CHECK(glUniform2f);
    CHECK(glUniform3f);
    CHECK(glUniform4f);
    CHECK(glUniform1i);
    CHECK(glUniform2i);
    CHECK(glUniform3i);
    CHECK(glUniform4i);
    CHECK(glUniformMatrix2fv);
    CHECK(glUniformMatrix3fv);
    CHECK(glUniformMatrix4fv);
    CHECK(glUseProgram);
    CHECK(glVertexAttrib1f);
    CHECK(glVertexAttrib2f);
    CHECK(glVertexAttrib3f);
    CHECK(glVertexAttrib4f);
    CHECK(glVertexAttribPointer);
    #undef CHECK

    /* Enable back-face culling; front-faces must be specified CCW
     * (CW faces get culled). */
    Renderer_ResetGlobalState();
    Renderer_ClearMatrices();

    Renderer_PushBlendMode(BlendMode::Disabled);
    Renderer_PushCullMode(CullMode::Backface);
    Renderer_PushZBuffer(true);
    Renderer_PushZWritable(true);

    /* Core profile (GL 3.3+) requires a Vertex Array Object to be bound for
     * any vertex attribute / buffer state to be valid. Bind a single global
     * VAO for the lifetime of the renderer so the engine's attribute setup
     * (which never created VAOs) has somewhere to live. */
    GL_BindVertexArray(GL_GenVertexArray());

    /* Bind a dummy SSBO at binding point 0 so non-instanced shaders
       never access an unbound buffer object. */
    GetDummyInstanceBuffer();

    /* Bind a dummy SSBO at binding point 1 for the particle instancing
       path (same reason — GLSL doesn't short-circuit). */
    GetParticleDummyBuffer();
  }

// ----------------------------------------------------------------------------

  static void InjectMatrices(ShaderT& shader) {
    /* Compute worldIT on demand — only when the shader actually uses it.
     * Most shaders don't need WORLDIT, so this avoids ~30K matrix inverses
     * per frame (moved from Renderer_SetWorldTransform). */
    Matrix worldIT;
    if (shader.QueryUniformLocation("WORLDIT") >= 0)
      worldIT = renderer.world.Inverse().Transpose();

    shader.BindMatrices(
      Renderer_GetWorldMatrix(),
      Renderer_GetViewMatrix(),
      Renderer_GetProjMatrix(),
      worldIT,
      Renderer_GetWorldViewProjMatrix());
  }

  static void PrepareMeshForDraw(MeshT const* mesh) {
    /* If the mesh has not yet been bound to a buffer object, we need to create
     * one and load in the data. Using VBOs minimizes data transfer between
     * the CPU and GPU. */
    if (mesh->bufferVersion != mesh->version) {
      mesh->bufferVersion = mesh->version;
      GL_DeleteBuffer(mesh->vbo);
      GL_DeleteBuffer(mesh->ibo);
      mesh->vbo = GL_NullBuffer;
      mesh->ibo = GL_NullBuffer;
    }

    if (mesh->vbo == GL_NullBuffer) {
      mesh->vbo = GL_GenBuffer();
      mesh->ibo = GL_GenBuffer();

      /* Need to force these bindings, as it is impossible that the new buffers
         are already bound. */
      Renderer_BindVertexBuffer(mesh->vbo, true);
      Renderer_BindIndexBuffer(mesh->ibo, true);

      GL_BufferData(
        GL_BufferTarget::Array,
        sizeof(Vertex) * mesh->vertices.size(),
        mesh->GetVertexPointer(),
        GL_BufferUsage::StaticDraw);

      /* If the indices can fit in unsigned short format, store them that way
         to save space. */
      if (kAllow16BitIndices && mesh->vertices.size() < USHRT_MAX) {
        static Vector<ushort> indices;
        indices.clear();
        for (uint i = 0; i < mesh->indices.size(); ++i)
          indices << (ushort)mesh->indices[i];

        mesh->indexFormat = GL_IndexFormat::Short;
        GL_BufferData(
          GL_BufferTarget::ElementArray,
          sizeof(ushort) * indices.size(),
          (void const*)indices.data(),
          GL_BufferUsage::StaticDraw);
      }

      /* Otherwise, fall back on standard unsigned int format. */
      else {
        mesh->indexFormat = GL_IndexFormat::Int;
        GL_BufferData(
          GL_BufferTarget::ElementArray,
          sizeof(uint) * mesh->indices.size(),
          mesh->GetIndexPointer(),
          GL_BufferUsage::StaticDraw);
      }
    }
  }

  static void SetBlendMode(BlendMode::Enum mode) {
    if (mode == BlendMode::Additive) {
      GL_Enable(GL_Capability::Blend);
      GL_BlendFuncSeparate(
        GL_BlendFunction::One,
        GL_BlendFunction::One,
        GL_BlendFunction::One,
        GL_BlendFunction::One);
    } else if (mode == BlendMode::Alpha) {
      GL_Enable(GL_Capability::Blend);
      GL_BlendFuncSeparate(
        GL_BlendFunction::SourceAlpha,
        GL_BlendFunction::OneMinusSourceAlpha,
        GL_BlendFunction::One,
        GL_BlendFunction::One);
    } else if (mode == BlendMode::Complementary) {
      GL_Enable(GL_Capability::Blend);
      GL_BlendFunc(
        GL_BlendFunction::One,
        GL_BlendFunction::OneMinusSourceColor);
    } else if (mode == BlendMode::Disabled) {
      GL_Disable(GL_Capability::Blend);
    } else if (mode == BlendMode::Multiplicative) {
      GL_Enable(GL_Capability::Blend);
      GL_BlendFunc(GL_BlendFunction::DestColor, GL_BlendFunction::Zero);
    }
  }

  inline static void SetZBuffer(bool useZBuffer) {
    if (useZBuffer)
      GL_Enable(GL_Capability::DepthTest);
    else
      GL_Disable(GL_Capability::DepthTest);
  }

  inline static void SetCullMode(CullMode::Enum mode) {
    if (mode == CullMode::Backface) {
      GL_Enable(GL_Capability::CullFace);
      GL_CullFace(GL_CullMode::Back);
    } else if (mode == CullMode::Frontface) {
      GL_Enable(GL_Capability::CullFace);
      GL_CullFace(GL_CullMode::Front);
    } else
      GL_Disable(GL_Capability::CullFace);
  }

  inline static void SetZWritable(bool zWritable) {
    GL_DepthMask(zWritable);
  }

// ----------------------------------------------------------------------------

  void Renderer_BindIndexBuffer(GL_Buffer buffer, bool force) {
    if (force || renderer.indexBuffer != buffer) {
      renderer.indexBuffer = buffer;
      GL_BindBuffer(GL_BufferTarget::ElementArray, renderer.indexBuffer);
    }
  }

  void Renderer_BindVertexBuffer(GL_Buffer buffer, bool force) {
    if (force || renderer.vertexBuffer != buffer) {
      renderer.vertexBuffer = buffer;
      GL_BindBuffer(GL_BufferTarget::Array, renderer.vertexBuffer);
    }
  }

  void Renderer_DisableAttribArray(uint index) {
    if (renderer.attribArrayEnabled[index]) {
      renderer.attribArrayEnabled[index] = false;
      GL_DisableVertexAttribArray(index);
    }
  }

  void Renderer_EnableAttribArray(uint index) {
    if (!renderer.attribArrayEnabled[index]) {
      renderer.attribArrayEnabled[index] = true;
      GL_EnableVertexAttribArray(index);
    }
  }

// ----------------------------------------------------------------------------

  void Renderer_Clear(V4 const& clearColor) {
    GL_ClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    GL_Clear(GL_BufferBit::Color);
    Renderer_ResetGlobalState();
  }

  void Renderer_ClearDepth(float depth) {
    GL_ClearDepth(depth);
    GL_Clear(GL_BufferBit::Depth);
  }

  void Renderer_ClearMatrices() {
    renderer.world =
    renderer.view =
    renderer.proj =
    renderer.wvp =
    Matrix::Identity();
  }

  void Renderer_DrawFSQ() {
    Renderer_PushBlendMode(BlendMode::Disabled);
    Renderer_PushZBuffer(false);
    Renderer_DrawQuad();
    Renderer_PopZBuffer();
    Renderer_PopBlendMode();
    renderer.callCount++;
  }

  void Renderer_DrawFSQInParts(uint width, uint height, uint parts) {
    GL_Enable(GL_Capability::ScissorTest);
    uint x = 0;
    uint w = width / parts;

    for (size_t i = 0; i < parts - 1; ++i) {
      GL_Scissor(x, 0, w, height);
      Renderer_DrawQuad();
      GL_Finish();
      x += w;
    }

    GL_Scissor(x, 0, (width - x), height);
    Renderer_DrawQuad();
    GL_Disable(GL_Capability::ScissorTest);
  }

  void Renderer_DrawFSQPart(uint x, uint y, uint width, uint height) {
    GL_Enable(GL_Capability::ScissorTest);
    GL_Scissor(x, y, width, height);
    Renderer_DrawQuad();
    GL_Disable(GL_Capability::ScissorTest);
  }

  void Renderer_DrawMesh(MeshT const* mesh) {
    PrepareMeshForDraw(mesh);

    Renderer_BindVertexBuffer(mesh->vbo);
    Renderer_BindIndexBuffer(mesh->ibo);

    Renderer_EnableAttribArray(0);
    Renderer_EnableAttribArray(1);
    Renderer_EnableAttribArray(2);

    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, p));
    GL_VertexAttribPointer(1, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, n));
    GL_VertexAttribPointer(2, 2, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, u));
    GL_DrawElements(
      GL_DrawMode::Triangles,
      mesh->GetIndices(),
      mesh->indexFormat,
      nullptr);

    renderer.callCount++;
    renderer.polyCount += mesh->GetIndices() / 3;
  }

  void Renderer_BeginInstancedDraw(Matrix const* interleavedData, int count) {
    LTE_ASSERT(count > 0);

    /* Upload interleaved world + worldIT matrices to the SSBO.
       The caller provides 2 * count matrices: { world, worldIT } per instance,
       matching the GLSL struct { mat4 world; mat4 worldIT; }. */
    GL_Buffer buf = GetInstanceBuffer();
    GL_BindBuffer(GL_BufferTarget::ShaderStorage, buf);

    size_t needed = (size_t)count;
    if (needed > gInstanceBufferCapacity) {
      /* Orphan and reallocate (standard streaming pattern).
         Capacity is in instance count (each instance = 2 mat4 = 128 bytes). */
      gInstanceBufferCapacity = needed * 2;
      GL_BufferData(GL_BufferTarget::ShaderStorage,
                    (int)(sizeof(Matrix) * 2 * gInstanceBufferCapacity),
                    nullptr, GL_BufferUsage::DynamicDraw);
    }
    GL_BufferSubData(GL_BufferTarget::ShaderStorage, 0,
                     (int)(sizeof(Matrix) * 2 * count), interleavedData);

    GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                      kInstanceBindingPoint, buf);
    /* NOTE: uInstanced is set in Renderer_DrawMeshInstanced, after the
       model's shader is bound via Begin() -> glUseProgram. Setting it
       here would apply to the WRONG shader (the one active before this
       call). */
  }

  void Renderer_EndInstancedDraw() {
    SetInstancedUniform(0);

    /* Rebind the dummy SSBO so non-instanced shaders don't touch the
       instance buffer. */
    GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                      kInstanceBindingPoint, GetDummyInstanceBuffer());
  }

  void Renderer_DrawMeshInstanced(MeshT const* mesh, int instanceCount) {
    PrepareMeshForDraw(mesh);

    /* Set uInstanced AFTER the model's shader is bound via Begin()->glUseProgram.
       This ensures the uniform is set on the correct (currently active) program. */
    SetInstancedUniform(1);

    Renderer_BindVertexBuffer(mesh->vbo);
    Renderer_BindIndexBuffer(mesh->ibo);

    Renderer_EnableAttribArray(0);
    Renderer_EnableAttribArray(1);
    Renderer_EnableAttribArray(2);

    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, p));
    GL_VertexAttribPointer(1, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, n));
    GL_VertexAttribPointer(2, 2, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, u));
    GL_DrawElementsInstanced(
      GL_DrawMode::Triangles,
      mesh->GetIndices(),
      mesh->indexFormat,
      nullptr,
      instanceCount);

    renderer.callCount++;
    renderer.polyCount += (mesh->GetIndices() / 3) * instanceCount;
  }

  void Renderer_DrawParticlesInstanced(
    MeshT const* mesh,
    ParticleInstanceData const* instances,
    int count)
  {
    LTE_ASSERT(count > 0);
    PrepareMeshForDraw(mesh);

    /* Upload per-particle instance data to SSBO at binding point 1.
       Layout: { vec4 posAndSize, vec4 ageAndColor } per instance = 32 bytes. */
    GL_Buffer buf = GetParticleInstanceBuffer();
    GL_BindBuffer(GL_BufferTarget::ShaderStorage, buf);

    size_t needed = (size_t)count;
    if (needed > gParticleInstanceBufferCapacity) {
      gParticleInstanceBufferCapacity = needed * 2;
      GL_BufferData(GL_BufferTarget::ShaderStorage,
                    (int)(sizeof(ParticleInstanceData) * gParticleInstanceBufferCapacity),
                    nullptr, GL_BufferUsage::DynamicDraw);
    }
    GL_BufferSubData(GL_BufferTarget::ShaderStorage, 0,
                     (int)(sizeof(ParticleInstanceData) * count),
                     instances);
    GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                      kParticleBindingPoint, buf);

    /* Set uParticleInstanced AFTER the shader is bound. */
    SetParticleInstancedUniform(1);

    Renderer_BindVertexBuffer(mesh->vbo);
    Renderer_BindIndexBuffer(mesh->ibo);

    Renderer_EnableAttribArray(0);
    Renderer_EnableAttribArray(1);
    Renderer_EnableAttribArray(2);

    /* The billboard mesh has Vertex layout: p(3), n(3), u(2), v(2), c(3).
       Positions are at origin (all zeros), UVs provide the quad corners.
       The particle vertex shader reads position/size/age/color from SSBO
       instead of these attributes when uParticleInstanced > 0. */
    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, p));
    GL_VertexAttribPointer(1, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, n));
    GL_VertexAttribPointer(2, 2, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, u));
    GL_DrawElementsInstanced(
      GL_DrawMode::Triangles,
      mesh->GetIndices(),
      mesh->indexFormat,
      nullptr,
      count);

    SetParticleInstancedUniform(0);

    /* Rebind the dummy SSBO so non-particle shaders don't touch the
       instance buffer. */
    GL_BindBufferBase(GL_BufferTarget::ShaderStorage,
                      kParticleBindingPoint, GetParticleDummyBuffer());

    renderer.callCount++;
    renderer.polyCount += (mesh->GetIndices() / 3) * count;
  }

  namespace {
    /* A single shared unit quad (pos + uv interleaved) used by Renderer_DrawQuad.
       Core profiles forbid client-side vertex arrays, so the quad lives in a VBO.
       The layout matches DrawQuad's attribute setup: attrib 0 = vec3 pos,
       attrib 2 = vec2 uv. */
    struct QuadVertex {
      float p[3];
      float t[2];
    };

    GL_Buffer GetSharedQuadVBO() {
      static GL_Buffer quadVBO = GL_NullBuffer;
      if (quadVBO == GL_NullBuffer) {
        static const QuadVertex quad[] = {
          { { 0.f, 0.f, 0.f }, { 0.f, 0.f } },
          { { 1.f, 0.f, 0.f }, { 1.f, 0.f } },
          { { 1.f, 1.f, 0.f }, { 1.f, 1.f } },
          { { 0.f, 1.f, 0.f }, { 0.f, 1.f } },
        };
        quadVBO = GL_GenBuffer();
        Renderer_BindVertexBuffer(quadVBO, true);
        GL_BufferData(
          GL_BufferTarget::Array,
          sizeof(quad),
          quad,
          GL_BufferUsage::DynamicDraw);
      }
      return quadVBO;
    }

    GL_Buffer GetSharedQuadIBO() {
      static GL_Buffer quadIBO = GL_NullBuffer;
      if (quadIBO == GL_NullBuffer) {
        static const uchar indices[] = { 0, 1, 2, 0, 2, 3 };
        quadIBO = GL_GenBuffer();
        Renderer_BindIndexBuffer(quadIBO, true);
        GL_BufferData(
          GL_BufferTarget::ElementArray,
          sizeof(indices),
          indices,
          GL_BufferUsage::StaticDraw);
      }
      return quadIBO;
    }
  }

  void Renderer_DrawQuad(
    V2 const& p1,
    V2 const& p2,
    V2 const& t1,
    V2 const& t2,
    float depth)
  {
    /* Positions are in world space; the shader applies world-view-proj (same
       as the original client-side-array path). The quad VBO is filled per-call
       because the rect is dynamic. */
    QuadVertex scratch[4] = {
      { { p1.x, p1.y, depth }, { t1.x, t1.y } },
      { { p2.x, p1.y, depth }, { t2.x, t1.y } },
      { { p2.x, p2.y, depth }, { t2.x, t2.y } },
      { { p1.x, p2.y, depth }, { t1.x, t2.y } },
    };

    Renderer_BindVertexBuffer(GetSharedQuadVBO(), true);
    Renderer_BindIndexBuffer(GetSharedQuadIBO(), true);
    Renderer_EnableAttribArray(0);
    Renderer_DisableAttribArray(1);
    Renderer_EnableAttribArray(2);

    GL_BufferSubData(
      GL_BufferTarget::Array,
      0,
      sizeof(scratch),
      scratch);

    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, sizeof(QuadVertex),
                           (void const*)offset_of(QuadVertex, p));
    GL_VertexAttribPointer(2, 2, GL_DataFormat::Float, false, sizeof(QuadVertex),
                           (void const*)offset_of(QuadVertex, t));
    GL_DrawElements(GL_DrawMode::Triangles, 6, GL_IndexFormat::Byte, nullptr);
    renderer.callCount++;
  }

  void Renderer_DrawQuadOutline(V2 const& p1, V2 const& p2) {
    /* Core profiles forbid immediate mode (glBegin/glVertex), so emit the
       loop as a dynamic VBO draw. */
    static GL_Buffer outlineVBO = GL_NullBuffer;
    if (outlineVBO == GL_NullBuffer)
      outlineVBO = GL_GenBuffer();

    float const v[4 * 3] = {
      p1.x, p1.y, 0.f,
      p1.x, p2.y, 0.f,
      p2.x, p2.y, 0.f,
      p2.x, p1.y, 0.f,
    };
    Renderer_BindVertexBuffer(outlineVBO, true);
    GL_BufferData(GL_BufferTarget::Array, sizeof(v), v, GL_BufferUsage::DynamicDraw);
    Renderer_EnableAttribArray(0);
    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, 0, nullptr);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    renderer.callCount++;
  }

  namespace {
    /* Persistent VBO/IBO used for one-off CPU-vertex draws (debug / arbitrary
       geometry). Core profiles forbid client-side vertex arrays, so we upload
       the CPU data each call and draw from the buffer. */
    GL_Buffer GetDebugVBO() {
      static GL_Buffer b = GL_NullBuffer;
      if (b == GL_NullBuffer)
        b = GL_GenBuffer();
      return b;
    }
    GL_Buffer GetDebugIBO() {
      static GL_Buffer b = GL_NullBuffer;
      if (b == GL_NullBuffer)
        b = GL_GenBuffer();
      return b;
    }
    size_t IndexByteSize(GL_IndexFormat::Enum fmt) {
      return fmt == GL_IndexFormat::Byte ? 1
           : fmt == GL_IndexFormat::Short ? 2 : 4;
    }
  }

  void StaticDrawVertices(
    Vertex const* vertexData,
    size_t vertexCount,
    void const* indexData,
    GL_IndexFormat::Enum indexFormat,
    size_t indexCount)
  {
    Renderer_BindVertexBuffer(GetDebugVBO(), true);
    GL_BufferData(GL_BufferTarget::Array,
                  (GLsizei)(sizeof(Vertex) * vertexCount),
                  (void const*)vertexData, GL_BufferUsage::DynamicDraw);
    Renderer_BindIndexBuffer(GetDebugIBO(), true);
    GL_BufferData(GL_BufferTarget::ElementArray,
                  (GLsizei)(IndexByteSize(indexFormat) * indexCount),
                  indexData, GL_BufferUsage::DynamicDraw);

    Renderer_EnableAttribArray(0);
    Renderer_EnableAttribArray(1);
    Renderer_EnableAttribArray(2);

    GL_VertexAttribPointer(0, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, p));
    GL_VertexAttribPointer(1, 3, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, n));
    GL_VertexAttribPointer(2, 2, GL_DataFormat::Float, false, sizeof(Vertex),
                           (void const*)offset_of(Vertex, u));

    GL_DrawElements(GL_DrawMode::Triangles, indexCount, indexFormat, nullptr);
    renderer.callCount++;
    renderer.polyCount += indexCount / 3;
  }

  void Renderer_DrawVertices(
    Vector<Vertex> const& vertices,
    Vector<uint> const& indices)
  {
    StaticDrawVertices(
      vertices.data(),
      vertices.size(),
      indices.data(),
      GL_IndexFormat::Int,
      indices.size());
  }

  void Renderer_DrawVertices(
    Vector<Vertex> const& vertices,
    Vector<ushort> const& indices)
  {
    StaticDrawVertices(
      vertices.data(),
      vertices.size(),
      indices.data(),
      GL_IndexFormat::Short,
      indices.size());
  }

  namespace {
    /* Find the maximum index referenced so we know how many vertices to upload
       for a client-side (CPU) vertex array. */
    uint MaxIndex(void const* indexData, uint count, GL_IndexFormat::Enum fmt) {
      uint max = 0;
      if (fmt == GL_IndexFormat::Byte) {
        uchar const* p = (uchar const*)indexData;
        for (uint i = 0; i < count; ++i) max = Max(max, (uint)p[i]);
      } else if (fmt == GL_IndexFormat::Short) {
        ushort const* p = (ushort const*)indexData;
        for (uint i = 0; i < count; ++i) max = Max(max, (uint)p[i]);
      } else {
        uint const* p = (uint const*)indexData;
        for (uint i = 0; i < count; ++i) max = Max(max, p[i]);
      }
      return max;
    }
  }

  void Renderer_DrawVertices(
    void const* vertexData,
    Type const& vertexFormat,
    void const* indexData,
    uint indices,
    GL_IndexFormat::Enum indexFormat)
  {
    /* Core profiles forbid client-side vertex arrays, so upload the referenced
       vertices + indices to VBOs, then bind attrib pointers by offset. */
    uint maxIdx = MaxIndex(indexData, indices, indexFormat);
    size_t vertexBytes = (size_t)(maxIdx + 1) * vertexFormat->size;

    Renderer_BindVertexBuffer(GetDebugVBO(), true);
    GL_BufferData(GL_BufferTarget::Array, (GLsizei)vertexBytes,
                  vertexData, GL_BufferUsage::DynamicDraw);
    Renderer_BindIndexBuffer(GetDebugIBO(), true);
    GL_BufferData(GL_BufferTarget::ElementArray,
                  (GLsizei)(IndexByteSize(indexFormat) * indices),
                  indexData, GL_BufferUsage::DynamicDraw);

    uint attribs = vertexFormat->GetFieldCount((void*)vertexData);
    for (uint i = 0; i < attribs; ++i) {
      FieldType field = vertexFormat->GetField((void*)vertexData, i);
      Renderer_EnableAttribArray(i);

      uint components = 0;
      if (field.type == Type_Get<float>())
        components = 1;
      else if (field.type == Type_Get<V2>())
        components = 2;
      else if (field.type == Type_Get<V3>())
        components = 3;
      else if (field.type == Type_Get<V4>())
        components = 4;
      else
        error("Vertex format must contain only float, vec2, vec3, or vec4 types");

      GL_VertexAttribPointer(
        i,
        components,
        GL_DataFormat::Float,
        false,
        vertexFormat->size,
        (void const*)((char const*)field.address - (char const*)vertexData));
    }

    GL_DrawElements(GL_DrawMode::Triangles, indices, indexFormat, nullptr);

    for (uint i = 3; i < attribs; ++i)
      Renderer_DisableAttribArray(i);

    renderer.callCount++;
    renderer.polyCount += indices / 3;
  }

  void Renderer_Flush() {
    GL_Finish();
  }

  int Renderer_GetDrawCallCount() {
    return renderer.callCountLast;
  }

  int Renderer_GetPolyCount() {
    return renderer.polyCountLast;
  }

  void Renderer_ResetCounters() {
    renderer.callCountLast = renderer.callCount;
    renderer.polyCountLast = renderer.polyCount;
    renderer.callCount = 0;
    renderer.polyCount = 0;
  }

  void Renderer_SetColor(Color const& color, float alpha) {
    GL_Color(color.x, color.y, color.z, alpha);
  }

  void Renderer_SetShader(ShaderT& shader) {
    shader.Use();
    InjectMatrices(shader);
  }

  void Renderer_SetShaderFF() {
    Shader_UseFixedFunction();
  }

  void Renderer_SetViewport(V2 const& origin, V2 const& size) {
    GL_Viewport((int)origin.x, (int)origin.y, (size_t)size.x, (size_t)size.y);
  }

  void Renderer_SetWireframe(bool wireframe) {
    if (wireframe)
      GL_PolygonMode(GL_FaceType::FrontAndBack, GL_FillMode::Line);
    else
      GL_PolygonMode(GL_FaceType::FrontAndBack, GL_FillMode::Fill);
  }

  void Renderer_PushBlendMode(BlendMode::Enum mode) {
    renderer.blendMode.push(mode);
    SetBlendMode(mode);
  }

  void Renderer_PopBlendMode() {
    renderer.blendMode.pop();
    SetBlendMode(renderer.blendMode.back());
  }

  void Renderer_PushCullMode(CullMode::Enum mode) {
    renderer.cullMode.push(mode);
    SetCullMode(mode);
  }

  void Renderer_PopCullMode() {
    renderer.cullMode.pop();
    SetCullMode(renderer.cullMode.back());
  }

  void Renderer_PopAllBuffers() {
    for (size_t i = 0; i < kMaxColorAttachments; ++i)
      Renderer_PopColorBuffer(i);
    Renderer_PopDepthBuffer();
  }

  void Renderer_PushAllBuffers() {
    for (size_t i = 0; i < kMaxColorAttachments; ++i)
      Renderer_PushColorBuffer(i, GL_NullTexture, 0);
    Renderer_PushDepthBuffer(GL_NullTexture);
  }

  void Renderer_PopColorBuffers() {
    for (size_t i = 0; i < kMaxColorAttachments; ++i)
      Renderer_PopColorBuffer(i);
  }

  void Renderer_PushColorBuffers() {
    for (size_t i = 0; i < kMaxColorAttachments; ++i)
      Renderer_PushColorBuffer(i, GL_NullTexture, 0);
  }

  void Renderer_PushColorBuffer(
    uint index,
    GL_Texture buffer,
    uint guid,
    GL_TextureTarget::Enum target)
  {
    LTE_ASSERT(index < kMaxColorAttachments);
    renderer.colorAttachment[index].push(Attachment(buffer, target, guid));
    Renderer_UpdateFramebuffer();
  }

  void Renderer_PopColorBuffer(uint index) {
    LTE_ASSERT(index < kMaxColorAttachments);
    renderer.colorAttachment[index].pop();
    Renderer_UpdateFramebuffer();
  }

  void Renderer_PushDepthBuffer(GL_Texture buffer) {
    renderer.depthAttachment.push(buffer);
    Renderer_UpdateFramebuffer();
  }

  void Renderer_PopDepthBuffer() {
    renderer.depthAttachment.pop();
    Renderer_UpdateFramebuffer();
  }

  void Renderer_PushScissorOff() {
    renderer.scissor.push(Tuple(false, 0, 0));
    GL_Disable(GL_Capability::ScissorTest);
  }

  void Renderer_PushScissorOn(V2 const& origin, V2 const& size) {
    renderer.scissor.push(Tuple(true, origin, size));
    GL_Enable(GL_Capability::ScissorTest);
    GL_Scissor((int)origin.x, (int)origin.y, (int)size.x, (int)size.y);
  }

  void Renderer_PopScissor() {
    renderer.scissor.pop();
    if (renderer.scissor.empty() || !renderer.scissor.back().x)
      GL_Disable(GL_Capability::ScissorTest);
    else {
      GL_Enable(GL_Capability::ScissorTest);
      V2 origin = renderer.scissor.back().y;
      V2 size = renderer.scissor.back().z;
      GL_Scissor((int)origin.x, (int)origin.y, (int)size.x, (int)size.y);
    }
  }

  void Renderer_PushViewport(int x, int y, int w, int h) {
    renderer.viewport.push(V4T<int>(x, y, w, h));
    GL_Viewport(x, y, w, h);
  }

  void Renderer_PopViewport() {
    renderer.viewport.pop();
    if (renderer.viewport.size()) {
      V4T<int> const& vp = renderer.viewport.back();
      GL_Viewport(vp.x, vp.y, vp.z, vp.w);
    }
  }

  void Renderer_PushZBuffer(bool useZBuffer) {
    renderer.zBuffer.push(useZBuffer);
    SetZBuffer(useZBuffer);
  }

  void Renderer_PopZBuffer() {
    renderer.zBuffer.pop();
    SetZBuffer(renderer.zBuffer.back());
  }

  void Renderer_PopZWritable() {
    renderer.zWritable.pop();
    SetZWritable(renderer.zWritable.back());
  }

  void Renderer_PushZWritable(bool zWritable) {
    renderer.zWritable.push(zWritable);
    SetZWritable(zWritable);
  }

  void Renderer_SetWorldTransform(Transform const& world) {
    Transform newTransform = world;
    if (kCameraSpaceRendering)
      newTransform.pos -= renderer.viewTransform.pos;
    renderer.world = newTransform.GetMatrix();
    renderer.wvp = renderer.proj * (renderer.view * renderer.world);
  }

  void Renderer_SetViewTransform(Transform const& view) {
    Transform newTransform = view;
    if (kCameraSpaceRendering)
      newTransform.pos = 0;
    renderer.viewTransform = view;
    renderer.view = newTransform.GetMatrix().Inverse();
    renderer.wvp = renderer.proj * (renderer.view * renderer.world);
  }

  void Renderer_SetProjMatrix(Matrix const& proj) {
    renderer.proj = proj;
    renderer.wvp = renderer.proj * (renderer.view * renderer.world);
  }

  Matrix const& Renderer_GetWorldMatrix() {
    return renderer.world;
  }

  Matrix const& Renderer_GetWorldViewProjMatrix() {
    return renderer.wvp;
  }

  Matrix const& Renderer_GetViewMatrix() {
    return renderer.view;
  }

  Matrix const& Renderer_GetProjMatrix() {
    return renderer.proj;
  }
}
