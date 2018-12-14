#ifndef RENDER_PASS_H
#define RENDER_PASS_H

/*
 * For students:
 * 
 * This file defines RenderPass class and its associated classes.
 * RenderPass is used to simplify multi-pass rendering code in the
 * reference solution.
 * 
 * However, understanding this class is COMPLETELY OPTIONAL.
 * It's totally OK to ignore this class and cram a bunch of OpenGL
 * function calls in your solution.
 */

#include <vector>
#include <map>
#include <functional>
#include "shader_uniform.h"
#include "debuggl.h"

struct RenderInputMeta;

/*
 * RenderDataInput: describe per-vertex attribute buffers used by RenderPass
 */
class RenderDataInput {
public:
	RenderDataInput();
	~RenderDataInput();

	/*
	 * assign: assign per-vertex attribute data
	 *      position: glVertexAttribPointer position 
	 *      name: glBindAttribLocation name
	 *      nelements: number of elements
	 *      element_length: element dimension, e.g. for vec3 it's 3
	 *      element_type: GL_FLOAT or GL_UNSIGNED_INT
     *      normalize: data normalization boolean
	 */
	void assign(int position,
	            const std::string& name,
	            const void *data,
	            size_t nelements,
	            size_t element_length,
	            int element_type,
                bool normalize);
	/*
	 * assign_index: assign the index buffer for vertices
	 * This will bind the data to GL_ELEMENT_ARRAY_BUFFER
	 * The element must be uvec3.
	 */
	void assignIndex(const void *data, size_t nelements, size_t element_length);

	int getNBuffers() const;
	const RenderInputMeta& getBufferMeta(int i) const;
	bool hasIndex() const { return has_index_; }
	const RenderInputMeta& getIndexMeta() const { return *index_meta_; }

private:
	std::vector<RenderInputMeta> meta_;
	std::shared_ptr<RenderInputMeta> index_meta_;
	bool has_index_ = false;
};

class RenderPass {
public:
	/*
	 * Constructor
	 *      vao: the Vertex Array Object, pass -1 to create new
	 *      input: RenderDataInput object
	 *      shaders: array of shaders, leave the second as nullptr if no GS present
	 *      uniforms: array of ShaderUniform objects
	 *      output: the FS output variable name.
	 * RenderPass does not support render-to-texture or multi-target
	 * rendering for now (and you also don't need it).
	 */
	RenderPass(int vao, // -1: create new VAO, otherwise use given VAO
	           const RenderDataInput& input,
	           const std::vector<const char*> shaders, // Order: VS, GS, FS 
	           const std::vector<ShaderUniformPtr> uniforms,
	           const std::vector<const char*> output // Order: 0, 1, 2...
		  );
	~RenderPass();

	unsigned getVAO() const { return unsigned(vao_); }
	void updateVBO(int position, const void* data, size_t nelement);
	void setup();
	/*
 	 * Note: here we don't have an unified render() function, because the
	 * reference solution renders with different primitives
	 * 
	 * However you can freely trianglize everything and add an
	 * render() function
	 */

	/*
	 * renderWithMaterial: render a part of vertex buffer, after binding
	 * corresponding uniforms for Phong shading.
	 */
	bool renderWithMaterial(int i); // return false if material id is invalid
private:
	void initMaterialUniform();
	void createMaterialTexture();

	int vao_;
	RenderDataInput input_;
	std::vector<ShaderUniformPtr> uniforms_;

	std::vector<unsigned> glbuffers_, unilocs_, malocs_;
	std::vector<unsigned> gltextures_, matexids_;
	unsigned sampler2d_;
	unsigned vs_ = 0, gs_ = 0, fs_ = 0;
	unsigned sp_ = 0;
	
	static unsigned compileShader(const char*, int type);
	static std::map<const char*, unsigned> shader_cache_;

	static void bindUniformsTo(std::vector<ShaderUniformPtr>& uniforms,
	                           const std::vector<unsigned>& unilocs);
};

#endif
