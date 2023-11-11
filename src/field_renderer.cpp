
#include "field_renderer.h"

#include <synapse/API>


//
FieldRendererBase::FieldRendererBase()
{
    //
    m_shader1D_BW = ShaderLibrary::load("../assets/shaders/field1d_BW.glsl");
    m_shader1D_RGB = ShaderLibrary::load("../assets/shaders/field1d_RGB.glsl");

    m_shader2D = ShaderLibrary::load("../assets/shaders/field2d.glsl");

    // create ndc quad
    // m_vpQuad = MeshCreator::createShapeViewportQuad();

}

//---------------------------------------------------------------------------------------
FieldRenderer::FieldRenderer(const glm::ivec2 &_sim_shape, const glm::ivec2 &_vp)
{
    assert(_sim_shape != glm::ivec2(0) && "shape not set");

    m_shape = _sim_shape;
    m_scalarTexture = std::make_shared<Texture2D>(_sim_shape.x, _sim_shape.y, ColorFormat::R32F);
    
    // calculate the ndc coordinates given that we have a square domain
    if (m_shape.x == m_shape.y)
    {
        float x_frac = (float)_vp.y / (float)_vp.x;
        m_vpQuad = MeshCreator::createShapeViewportQuadFraction({ -x_frac, -1 }, 
                                                                {  x_frac,  1 });
        m_xlim = { -x_frac, x_frac };
        m_ylim = { -1.0f, 1.0f };
    }
    else if (m_shape.x > m_shape.y)
    {
        m_vpQuad = MeshCreator::createShapeViewportQuad();
        m_xlim = { -1.0f, 1.0f };
        m_ylim = { -1.0f, 1.0f };
    }
    else
    {
        SYN_FATAL_ERROR("shape of scalar field not permitted (x > y), consider transposing?");
    }

    // allocate data buffers
    m_cellCount = _sim_shape.x * _sim_shape.y;
    m_data1D = new float[m_cellCount];
    m_data1DSzBytes = m_cellCount * sizeof(float);
    m_data2D = new glm::vec2[m_cellCount];
    m_data2DSzBytes = m_cellCount * sizeof(glm::vec2);

    // default shader
    m_shader1D = m_shader1D_RGB.get();

    //
    m_isInitialized = true;

}

//---------------------------------------------------------------------------------------
FieldRenderer::~FieldRenderer()
{
    if (m_data1D)   delete[] m_data1D;
    if (m_data2D)   delete[] m_data2D;
}

//---------------------------------------------------------------------------------------
void FieldRenderer::renderField1D()
{
    static auto &renderer = Renderer::get();

    if (m_scalarTexture == nullptr || !m_isInitialized)
        return;

    renderer.enableTexture2D(m_scalarTexture->getID());
    m_shader1D->enable();
    m_shader1D->setUniform1i("u_sampler", 0);
    m_vpQuad->renderNDC();

}

//---------------------------------------------------------------------------------------
void FieldRenderer::updateData2D()
{
    assert(m_data2D != nullptr && "no data in vector field ptr.");
    
    glm::vec2 V[m_cellCount * 2];
    glm::vec2 ndc_pos = { m_xlim[0], m_ylim[0] };

    float dx = (m_xlim[1] - m_xlim[0]) / (float)m_shape.x;
    float dy = (m_ylim[1] - m_ylim[0]) / (float)m_shape.y;

    glm::vec2 mid = { dx * 0.5f, dy * 0.5f };

    //
    for (int y = 0; y < m_shape.y; y++)
    {
        ndc_pos.x = m_xlim[0];
        for (int x = 0; x < m_shape.x; x++)
        {
            int idx = y * m_shape.x + x;
            glm::vec2 pos = ndc_pos + mid;
            V[2*idx+0] = pos;
            V[2*idx+1] = pos + m_data2D[idx];
            ndc_pos.x += dx;

        }
        ndc_pos.y += dy;

    }

    //
    Ref<VertexBuffer> vbo = API::newVertexBuffer(GL_STATIC_DRAW);
    vbo->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    vbo->setData(V, sizeof(V));

    //
    m_vao2D = API::newVertexArray(vbo);

}

//---------------------------------------------------------------------------------------
void FieldRenderer::renderField2D()
{
    static auto &renderer = Renderer::get();

    if (m_vao2D == nullptr)
        return;
    
    m_shader2D->enable();
    glPointSize(30.0f);
    renderer.drawArrays(m_vao2D, 2 * m_cellCount, 0, false, GL_LINES);
}

//---------------------------------------------------------------------------------------
void FieldRenderer::normalize_field_1d()
{
    m_dataRange1D = { MAXFLOAT, -MAXFLOAT };
    for (uint32_t i = 0; i < m_cellCount; i++)
    {
        const float val = m_data1D[i];
        m_dataRange1D[0] = min(m_dataRange1D[0], val);
        m_dataRange1D[1] = max(m_dataRange1D[1], val);
    }

    // normalize
    float inv_range = 1.0f / (m_dataRange1D[1] - m_dataRange1D[0]); 
    for (uint32_t i = 0; i < m_cellCount; i++)
        m_data1D[i] = (m_data1D[i] - m_dataRange1D[0]) * inv_range; 

}

