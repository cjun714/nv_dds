#pragma once

#include <string>
#include <deque>
#include <istream>

#include <assert.h>
#include <stdint.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3

namespace nv_dds {
struct DXTColBlock {
    uint16_t col0;
    uint16_t col1;

    uint8_t row[4];
};

struct DXT3AlphaBlock {
    uint16_t row[4];
};

struct DXT5AlphaBlock {
    uint8_t alpha0;
    uint8_t alpha1;

    uint8_t row[6];
};

enum TextureType {
    TextureNone, TextureFlat,    // 1D, 2D textures
    Texture3D,
    TextureCubemap
};

class CSurface {
public:
    CSurface();
    CSurface(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t *pixels);
    CSurface(const CSurface &copy);
    CSurface &operator=(const CSurface &rhs);
    virtual ~CSurface();

    operator uint8_t*() const;

    virtual void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t *pixels);
    virtual void clear();

    unsigned int get_width() const {
        return m_width;
    }
    unsigned int get_height() const {
        return m_height;
    }
    unsigned int get_depth() const {
        return m_depth;
    }
    unsigned int get_size() const {
        return m_size;
    }

private:
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_depth;
    unsigned int m_size;

    uint8_t *m_pixels;
};

class CTexture: public CSurface {
    friend class CDDSImage;

public:
    CTexture();
    CTexture(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t *pixels);
    CTexture(const CTexture &copy);
    CTexture &operator=(const CTexture &rhs);
    ~CTexture();

    void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const uint8_t *pixels);
    void clear();

    const CSurface &get_mipmap(unsigned int index) const {
        assert(!m_mipmaps.empty());
        assert(index < m_mipmaps.size());

        return m_mipmaps[index];
    }

    void add_mipmap(const CSurface &mipmap) {
        m_mipmaps.push_back(mipmap);
    }

    unsigned int get_num_mipmaps() const {
        return (unsigned int) m_mipmaps.size();
    }

protected:
    CSurface &get_mipmap(unsigned int index) {
        assert(!m_mipmaps.empty());
        assert(index < m_mipmaps.size());

        return m_mipmaps[index];
    }

private:
    std::deque<CSurface> m_mipmaps;
};

class CDDSImage {
public:
    CDDSImage();
    ~CDDSImage();

    void create_textureFlat(unsigned int format, unsigned int components, const CTexture &baseImage);
    void create_texture3D(unsigned int format, unsigned int components, const CTexture &baseImage);
    void create_textureCubemap(unsigned int format, unsigned int components, const CTexture &positiveX, const CTexture &negativeX, const CTexture &positiveY,
            const CTexture &negativeY, const CTexture &positiveZ, const CTexture &negativeZ);

    void clear();

    void load(std::istream& is, bool flipImage = true);
    void load(const std::string& filename, bool flipImage = true);
    void save(const std::string& filename, bool flipImage = true);

#ifndef GL_ES_VERSION_2_0
    bool upload_texture1D();
#endif

    bool upload_texture2D(unsigned int imageIndex = 0, GLenum target = GL_TEXTURE_2D);

#if !defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0)
    bool upload_texture3D();
#endif

    bool upload_textureCubemap();

    operator uint8_t*() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0];
    }

    unsigned int get_width() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0].get_width();
    }

    unsigned int get_height() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0].get_height();
    }

    unsigned int get_depth() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0].get_depth();
    }

    unsigned int get_size() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0].get_size();
    }

    unsigned int get_num_mipmaps() {
        assert(m_valid);
        assert(!m_images.empty());

        return m_images[0].get_num_mipmaps();
    }

    const CSurface &get_mipmap(unsigned int index) const {
        assert(m_valid);
        assert(!m_images.empty());
        assert(index < m_images[0].get_num_mipmaps());

        return m_images[0].get_mipmap(index);
    }

    const CTexture &get_cubemap_face(unsigned int face) const {
        assert(m_valid);
        assert(!m_images.empty());
        assert(m_images.size() == 6);
        assert(m_type == TextureCubemap);
        assert(face < 6);

        return m_images[face];
    }

    unsigned int get_components() {
        return m_components;
    }
    unsigned int get_format() {
        return m_format;
    }
    TextureType get_type() {
        return m_type;
    }

    bool is_compressed() {
        return (m_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
                || (m_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
                || (m_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
    }

    bool is_cubemap() {
        return (m_type == TextureCubemap);
    }
    bool is_volume() {
        return (m_type == Texture3D);
    }
    bool is_valid() {
        return m_valid;
    }

    bool is_dword_aligned() {
        assert(m_valid);

        int dwordLineSize = get_dword_aligned_linesize(get_width(), m_components * 8);
        int curLineSize = get_width() * m_components;

        return (dwordLineSize == curLineSize);
    }

private:
    unsigned int clamp_size(unsigned int size);
    unsigned int size_dxtc(unsigned int width, unsigned int height);
    unsigned int size_rgb(unsigned int width, unsigned int height);
    void swap_endian(void *val);

    // calculates 4-byte aligned width of image
    unsigned int get_dword_aligned_linesize(unsigned int width, unsigned int bpp) {
        return ((width * bpp + 31) & -32) >> 3;
    }

    void flip(CSurface &surface);
    void flip_texture(CTexture &texture);

    void flip_blocks_dxtc1(DXTColBlock *line, unsigned int numBlocks);
    void flip_blocks_dxtc3(DXTColBlock *line, unsigned int numBlocks);
    void flip_blocks_dxtc5(DXTColBlock *line, unsigned int numBlocks);
    void flip_dxt5_alpha(DXT5AlphaBlock *block);

    void write_texture(const CTexture &texture, std::ostream& os);

    unsigned int m_format;
    unsigned int m_components;
    TextureType m_type;
    bool m_valid;

    std::deque<CTexture> m_images;
};
}
