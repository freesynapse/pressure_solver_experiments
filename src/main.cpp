
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

    std::shared_ptr<Field1D> m_scalarField = nullptr;
    std::shared_ptr<Field2D> m_vectorField = nullptr;
    std::shared_ptr<FieldRenderer> m_fieldRenderer = nullptr;
    glm::ivec2 m_simShape = { 32, 32 };
    void onResize(Event *_e);
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
    m_fieldRenderer = std::make_shared<FieldRenderer>(m_simShape, e->getViewport());
    m_fieldRenderer->setData1D(m_scalarField);
    m_fieldRenderer->setData2D(m_vectorField);
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

    SYN_TRACE("\n\n", m_simShape.x * m_simShape.y, "\n\n");

    m_scalarField = std::make_shared<Field1D>(m_simShape);
    m_vectorField = std::make_shared<Field2D>(m_simShape);

    float *f = m_scalarField->data();
    for (int y = 0; y < m_simShape.y; y++)
        for (int x = 0; x < m_simShape.x; x++)
            f[y * m_simShape.x + x] = Random::rand_fC();

    glm::vec2 *v = m_vectorField->data();
    float theta = 0.0f;
    float theta_inc = 2*M_PI / (float)(m_simShape.x * m_simShape.y);
    
    for (int y = 0; y < m_simShape.y; y++)
    {
        for (int x = 0; x < m_simShape.x; x++)
        {            
            v[y * m_simShape.x + x] = glm::vec2(cosf(theta), sinf(theta)) * Random::rand_fC_r(0.2f, 1.0f);
            theta += theta_inc;
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
        // m_fieldRenderer->renderField1D();
        m_fieldRenderer->renderField2D(_dt);
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
