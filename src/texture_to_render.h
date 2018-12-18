#ifndef TEXTURE_TO_RENDER_H
#define TEXTURE_TO_RENDER_H

class TextureToRender {
public:
	TextureToRender();
	~TextureToRender();
	void create(int width, int height, bool render_depth=false);
	void bind();
	void unbind();
	int getTexture() const { return tex_; }
private:
	int w_, h_;
	unsigned int fb_ = -1;
	unsigned int tex_ = -1;
	unsigned int dep_ = -1;
};

#endif

