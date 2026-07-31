#ifndef GLType_h__
#define GLType_h__

template <class T, int N>
struct ExplicitTypedef {
  T data;
  ExplicitTypedef() = default;
  explicit ExplicitTypedef(T data) : data(data) {}
  operator T() const {return data;}
};

using GL_Buffer = ExplicitTypedef<unsigned int, 0>;
using GL_Framebuffer = ExplicitTypedef<unsigned int, 1>;
using GL_Program = ExplicitTypedef<unsigned int, 2>;
using GL_Renderbuffer = ExplicitTypedef<unsigned int, 3>;
using GL_Shader = ExplicitTypedef<unsigned int, 4>;
using GL_Texture = ExplicitTypedef<unsigned int, 5>;
using GL_VertexArray = ExplicitTypedef<unsigned int, 6>;

static const GL_Buffer       GL_NullBuffer(0);
static const GL_Framebuffer  GL_NullFramebuffer(0);
static const GL_Program      GL_NullProgram(0);
static const GL_Renderbuffer GL_NullRenderbuffer(0);
static const GL_Shader       GL_NullShader(0);
static const GL_Texture      GL_NullTexture(0);
static const GL_VertexArray  GL_NullVertexArray(0);

#endif
