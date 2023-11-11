#ifndef __Field_H
#define __Field_H

#include <synapse/Renderer>
using namespace Syn;

#include "field.h"


class FieldRendererBase
{
public:
    FieldRendererBase();
    ~FieldRendererBase() = default;

protected:
    Ref<Shader> m_shader1D_BW   = nullptr;
    Ref<Shader> m_shader1D_RGB  = nullptr;
    Ref<Shader> m_shader2D      = nullptr;
    Ref<MeshShape> m_vpQuad     = nullptr;

};

//
class FieldRenderer : public FieldRendererBase
{
public:
    friend class Fluid;

public:
    FieldRenderer() {}
    FieldRenderer(const glm::ivec2 &_sim_shape, const glm::ivec2 &_vp);
    ~FieldRenderer();

    //
    void renderField1D();
    void renderField2D();
    
    // Sets the scalar field data pointer
    __always_inline void setData1D(const std::shared_ptr<Field1D> &_field_1d) { setData1D(_field_1d.get()); }
    __always_inline void setData1D(Field<float> *_field_1d)
    {
        memcpy(m_data1D, _field_1d->data(), _field_1d->size_bytes());
        normalize_field_1d();
        updateData1D();
    }

    __always_inline void setNormalizedData1D(const std::shared_ptr<Field1D> &_field_1d) { setNormalizedData1D(_field_1d.get()); }
    __always_inline void setNormalizedData1D(Field1D *_field_1d)
    {
        memcpy(m_data1D, _field_1d->data(), _field_1d->size_bytes());
        updateData1D();
    }

    // Sets the vector field data pointer
    __always_inline void setData2D(std::shared_ptr<Field2D> _field_2d) { setData2D(_field_2d.get()); }
    __always_inline void setData2D(Field2D *_field_2d)
    {
        memcpy(m_data2D, _field_2d->data(), _field_2d->size_bytes());
        updateData2D();
    }
    
    //
    __always_inline void updateData1D() { m_scalarTexture->setData(m_data1D, m_data1DSzBytes); }
    void updateData2D();

    //
    // __always_inline void setShaderBW() { m_shader1D = m_shader1D_BW.get(); }
    // __always_inline void setShaderRGB() { m_shader1D = m_shader1D_RGB.get(); }
    void toggleScalarRGBShading()
    {
        if (m_useScalarRGBShader)
            m_shader1D = m_shader1D_BW.get();
        else
            m_shader1D = m_shader1D_RGB.get();
        
        m_useScalarRGBShader = !m_useScalarRGBShader;
    }

private:
    void normalize_field_1d();


private:

    Shader *m_shader1D              = nullptr;
    bool m_useScalarRGBShader       = false;
    Ref<Texture2D> m_scalarTexture  = nullptr;
    
    float *m_data1D                 = nullptr;
    glm::vec2 m_dataRange1D         = { 0.0f, 0.0f };
    uint32_t m_data1DSzBytes        = 0;

    glm::vec2 *m_data2D             = nullptr;
    uint32_t m_data2DSzBytes        = 0;
    Ref<VertexArray> m_vao2D        = nullptr;


    glm::ivec2 m_shape              = { 0, 0 };
    uint32_t m_cellCount            = 0;
    glm::vec2 m_xlim                = { 0, 0 };
    glm::vec2 m_ylim                = { 0, 0 };

    bool m_isInitialized            = false;

};




#endif // __Field_H
