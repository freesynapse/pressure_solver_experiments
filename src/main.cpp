
#include <synapse/Synapse>
#include <synapse/SynapseMain.hpp>

using namespace Syn;

#include "field.h"
#include "field_renderer.h"


//
class layer : public Layer
{
public:
    layer() : Layer("layer") {}
    virtual ~layer() {}

    virtual void onAttach() override;
    virtual void onUpdate(float _dt) override;
    virtual void onImGuiRender() override;
    void onKeyDownEvent(Event *_e);
    void onMouseButtonEvent(Event *_e);

public:
    Ref<Framebuffer> m_renderBuffer = nullptr;
    Ref<Font> m_font = nullptr;
    // flags
    bool m_wireframeMode = false;
    bool m_toggleCulling = false;

    std::shared_ptr<Field2D> m_velocity = nullptr;
    std::shared_ptr<Field1D> m_divergence = nullptr;
    std::shared_ptr<FieldRenderer> m_fieldRenderer = nullptr;
    glm::ivec2 m_shape = { 40, 40 };
    void onResize(Event *_e);

    bool m_doRenderDiv = true;
    bool m_doRenderVelocity = true;
};

//
class syn_app_instance : public Application
{
public:
    syn_app_instance() { this->pushLayer(new layer); }
};
Application* CreateSynapseApplication() { return new syn_app_instance(); }

//----------------------------------------------------------------------------------------
void layer::onResize(Event *_e)
{
    ViewportResizeEvent *e = dynamic_cast<ViewportResizeEvent*>(_e);
    m_fieldRenderer = std::make_shared<FieldRenderer>(m_shape, e->getViewport());
    m_fieldRenderer->setData1D(m_divergence);
    m_fieldRenderer->setData2D(m_velocity);
}

//----------------------------------------------------------------------------------------
void layer::onAttach()
{
    // register event callbacks
    EventHandler::register_callback(EventType::INPUT_KEY, SYN_EVENT_MEMBER_FNC(layer::onKeyDownEvent));
    EventHandler::register_callback(EventType::INPUT_MOUSE_BUTTON, SYN_EVENT_MEMBER_FNC(layer::onMouseButtonEvent));

    EventHandler::push_event(new WindowToggleFullscreenEvent());

    m_renderBuffer = API::newFramebuffer(ColorFormat::RGBA16F, glm::ivec2(0), 1, true, true, "render_buffer");

    // load font
    m_font = MakeRef<Font>("../assets/ttf/JetBrainsMono-Medium.ttf", 14.0f);
    m_font->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    //
    EventHandler::register_callback(EventType::VIEWPORT_RESIZE, SYN_EVENT_MEMBER_FNC(layer::onResize));

    //
    Field1D in_data = Field1D(m_shape);
    m_velocity = std::make_shared<Field2D>(m_shape);

    // 1D data -- something to point to...
    float *f = in_data.data();
    for (int y = 0; y < m_shape.y; y++)
    {
        for (int x = 0; x < m_shape.x; x++)
        {
            //f[y * m_shape.x + x] = -(cosf(y * 2*M_PI / (float)m_shape.y) + \
            //                            cosf(x * 2*M_PI / (float)m_shape.x));
            float f_ = -(std::pow(y-(m_shape.y*0.5f), 2) + std::pow(x-(m_shape.x*0.5f), 2));
            f[y * m_shape.x + x] = f_;
        }
    }

    // Velocity from input data (2d monotonic decrease around origin)
    glm::vec2 *v = m_velocity->data();

    glm::vec2 dv2 = { 2.0f / (float)m_shape.x, 2.0f / (float)m_shape.y };
    int n = m_shape.x;

    // TODO : replace with separate loops for borders and interior
    for (int y = 0; y < m_shape.y; y++)
    {
        for (int x = 0; x < m_shape.x; x++)
        {         
            int idx = y * m_shape.x + x;
            
            // borders
            //
            // top
            if (y == 0 && x > 0 && x < m_shape.x - 1)
            {
                v[idx].x = (f[idx+1] - f[idx-1]) / dv2.x;
                v[idx].y = (f[idx+n] - f[idx]) / (0.5f * dv2.y);
            }
            // bottom
            else if (y == m_shape.y - 1 && x > 0 && x < m_shape.x - 1)
            {
                v[idx].x = (f[idx+1] - f[idx-1]) / dv2.x;
                v[idx].y = (f[idx] - f[idx-n]) / (0.5f * dv2.y);
            }
            // left
            else if (x == 0 && y > 0 && y < m_shape.y - 1)
            {
                v[idx].x = (f[idx+1] - f[idx]) / (0.5f * dv2.x);
                v[idx].y = (f[idx+n] - f[idx-n]) / dv2.y;
            }
            // right
            else if (x == m_shape.x - 1 && y > 0 && y < m_shape.y - 1)
            {
                v[idx].x = (f[idx] - f[idx-1]) / (0.5f * dv2.x);
                v[idx].y = (f[idx+n] - f[idx-n]) / dv2.y;
            }

            // interior
            //
            else if (x > 0 && x < m_shape.x - 1 && y > 0 && y < m_shape.y - 1)
            {
                v[idx].x = (f[idx+1] - f[idx-1]) / dv2.x;
                v[idx].y = (f[idx+n] - f[idx-n]) / dv2.y;
            }

        }
    
    }

    // set corners
    v[0] = 0.5f * (v[1] + v[m_shape.x]);
    v[(m_shape.y-1)*m_shape.x+0] = 0.5f * (v[(m_shape.y-1)*m_shape.x+1] + v[(m_shape.y-2)*m_shape.x+0]);
    v[m_shape.x-1] = 0.5f * (v[m_shape.x-2] + v[m_shape.x + m_shape.x-1]);
    v[(m_shape.y-1)*m_shape.x+m_shape.x-1] = 0.5f * (v[(m_shape.y-2)*m_shape.x+m_shape.x-1] + v[(m_shape.y-1)*m_shape.x+m_shape.x-2]);

    // Compute the divergence from the velocity field
    //
    m_divergence = std::make_shared<Field1D>(m_shape);
    f = m_divergence->data();
    float h = 1.0f / (float)m_shape.y;
    float div_mult = 1.0f / (2 * h);
    for (int y = 0; y < m_shape.y; y++)
    {
        for (int x = 0; x < m_shape.x; x++)
        {
            int idx = y * m_shape.x + x;

            if (x == 0 || y == 0 || x == m_shape.x - 1 || y == m_shape.y - 1)
                f[idx] = 0.0f;
            else
                f[idx] = div_mult * (v[idx+1].x - v[idx-1].x + v[idx+m_shape.x].y - v[idx-m_shape.x].y);
        }

    }

    // general settings
	Renderer::get().setClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	Renderer::get().disableImGuiUpdateReport();
    // Application::get().setMaxFPS(30.0f);
}

//----------------------------------------------------------------------------------------
void layer::onUpdate(float _dt)
{
    SYN_PROFILE_FUNCTION();
	
    static auto& renderer = Renderer::get();
    
    m_renderBuffer->bind();
    if (m_wireframeMode)
        renderer.enableWireFrame();    
    renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // -- BEGINNING OF SCENE -- //
    

    if (m_fieldRenderer)
    {
        if (m_doRenderDiv)
            m_fieldRenderer->renderField1D();
        if (m_doRenderVelocity)
        m_fieldRenderer->renderField2D();
    }


    // -- END OF SCENE -- //


    if (m_wireframeMode)
        renderer.disableWireFrame();

	
    // Text rendering 
    // TODO: all text rendering should go into an overlay layer.
    static float fontHeight = m_font->getFontHeight() + 1.0f;
    int i = 0;
    m_font->beginRenderBlock();
	m_font->addString(2.0f, fontHeight * ++i, "fps=%.0f  VSYNC=%s", TimeStep::getFPS(), Application::get().getWindow().isVSYNCenabled() ? "ON" : "OFF");
    m_font->endRenderBlock();

    //
    m_renderBuffer->bindDefaultFramebuffer();
}
 
//----------------------------------------------------------------------------------------
void layer::onKeyDownEvent(Event *_e)
{
    KeyDownEvent *e = dynamic_cast<KeyDownEvent*>(_e);
    static bool vsync = true;

    if (e->getAction() == SYN_KEY_PRESSED)
    {
        switch (e->getKey())
        {
            case SYN_KEY_Z: vsync = !vsync; Application::get().getWindow().setVSYNC(vsync); break;
            case SYN_KEY_V:         m_renderBuffer->saveAsPNG(); break;
            case SYN_KEY_ESCAPE:    EventHandler::push_event(new WindowCloseEvent()); break;
            case SYN_KEY_F4:        m_wireframeMode = !m_wireframeMode; break;
            case SYN_KEY_F5:        m_toggleCulling = !m_toggleCulling; Renderer::setCulling(m_toggleCulling); break;

            case SYN_KEY_1:
                m_doRenderVelocity = !m_doRenderVelocity;
                break;
            
            case SYN_KEY_2:
                m_doRenderDiv = !m_doRenderDiv;
                break;
                
            default: break;
        }
    }
    
}

//----------------------------------------------------------------------------------------
void layer::onMouseButtonEvent(Event *_e)
{
    MouseButtonEvent *e = dynamic_cast<MouseButtonEvent *>(_e);

    switch (e->getButton())
    {
        case SYN_MOUSE_BUTTON_1:    break;
        case SYN_MOUSE_BUTTON_2:    break;
        default: break;
    }
    
}

//----------------------------------------------------------------------------------------
void layer::onImGuiRender()
{
    static bool p_open = true;

    static bool opt_fullscreen_persistant = true;
    static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
    bool opt_fullscreen = opt_fullscreen_persistant;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
    	ImGuiViewport* viewport = ImGui::GetMainViewport();
    	ImGui::SetNextWindowPos(viewport->Pos);
    	ImGui::SetNextWindowSize(viewport->Size);
    	ImGui::SetNextWindowViewport(viewport->ID);
    	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (opt_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	    window_flags |= ImGuiWindowFlags_NoBackground;

    window_flags |= ImGuiWindowFlags_NoTitleBar;

    ImGui::GetCurrentContext()->NavWindowingToggleLayer = false;

    //-----------------------------------------------------------------------------------
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("synapse-core", &p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
	    ImGui::PopStyleVar(2);

    // Dockspace
    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = 0;
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        dockspace_id = ImGui::GetID("dockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
    }
	
    //-----------------------------------------------------------------------------------
    // set the 'rest' of the window as the viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("synapse-core::renderer");
    static ImVec2 oldSize = { 0, 0 };
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x != oldSize.x || viewportSize.y != oldSize.y)
    {
        // dispatch a viewport resize event -- registered classes will receive this.
        EventHandler::push_event(new ViewportResizeEvent(glm::vec2(viewportSize.x, viewportSize.y)));
        SYN_CORE_TRACE("viewport [ ", viewportSize.x, ", ", viewportSize.y, " ]");
        oldSize = viewportSize;
    }

    // direct ImGui to the framebuffer texture
    ImGui::Image((void*)m_renderBuffer->getColorAttachmentIDn(0), viewportSize, { 0, 1 }, { 1, 0 });

    ImGui::End();
    ImGui::PopStyleVar();


    // end root
    ImGui::End();

}
