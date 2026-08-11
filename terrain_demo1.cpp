/// collisions 

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef _WIN64
#include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>

#include "ogldev_util.h"
#include "ogldev_basic_glfw_camera.h"
#include "ogldev_glfw.h"

#include "demo_config.h"
#include "texture_config.h"
#include "midpoint_disp_terrain.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);

static int g_seed = 0;

int gShowPoints;


class TerrainDemo10
{
public:

    TerrainDemo10()
    {}

    virtual ~TerrainDemo10()
    {
        SAFE_DELETE(m_pGameCamera);
    }


    void Init()
    {
        CreateWindow_(); // added '_' because of conflict with Windows.h

        InitCallbacks();

        InitTerrain();

        InitCamera();

        InitGUI();
    }


    void Run()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (m_showGui) {
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Terrain Demo 5");                          // Create a window called "Hello, world!" and append into it.

                ImGui::SliderFloat("Max height", &this->m_maxHeight, 0.0f, 1000.0f);
                ImGui::SliderFloat("Terrain roughness", &this->m_roughness, 0.0f, 5.0f);

                static float Height0 = 64.0f;
                static float Height1 = 128.0f;
                static float Height2 = 192.0f;
                static float Height3 = 256.0f;

                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);

                if (ImGui::Button("Generate")) {
                    m_terrain.Destroy();
                    SRANDOM;
                    m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);
                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
                }

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();

                // Rendering
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            RenderScene();

            glfwSwapBuffers(window);
        }
    }


    void RenderScene()
    {
        if (!m_showGui) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

      

        // Smoothly transition bob intensity
        m_targetBobIntensity = (m_isMoving && !m_isPaused) ? 1.0f : 0.0f;
        m_currentBobIntensity += (m_targetBobIntensity - m_currentBobIntensity) * m_bobIntensitySmoothFactor;

        if (m_currentBobIntensity > 0.001f) {  // Small threshold to avoid floating point issues
            Vector3f camPos = m_pGameCamera->GetPos();
            Vector3f camTarget = m_pGameCamera->GetTarget();

            // Use deltaTime if available, otherwise approximate
            float dt = 0.016f; // 60fps approximation
            m_bobTimer += dt * m_currentBobIntensity;

            // More complex bob pattern (combining multiple sine waves)
            float verticalBob = sin(m_bobTimer * m_bobFrequency) * m_bobAmplitude * m_currentBobIntensity;
            verticalBob += sin(m_bobTimer * m_bobFrequency * 2.0f) * m_bobAmplitude * 0.25f * m_currentBobIntensity;

            float horizontalBob = cos(m_bobTimer * m_bobFrequency * 0.5f) * m_bobHorizontalAmount * m_currentBobIntensity;

            // Apply to camera
            Vector3f camUp = m_pGameCamera->GetUp();
            Vector3f camForward = camTarget - camPos;
            camForward.Normalize();
            Vector3f camRight = camForward.Cross(camUp);
            camRight.Normalize();

            Vector3f newPos = camPos;
            newPos.y += verticalBob;
            newPos.x += camRight.x * horizontalBob;
            newPos.z += camRight.z * horizontalBob;

            m_pGameCamera->SetPosition(newPos);
        }
        else {
            m_bobTimer = 0.0f;
        }

        static float foo = 0.0f;
        foo += 0.002f;

        /*  float S = (float)m_terrainSize;
          float R = 2.5f * S;

          Vector3f Pos(S + cosf(foo) * R, m_maxHeight + 250.0f, S + sinf(foo) * R);
          m_pGameCamera->SetPosition(Pos);

          Vector3f Center(S, Pos.y * 0.50f, S);
          Vector3f Target = Center - Pos;
          m_pGameCamera->SetTarget(Target);
          m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);*/

        //float y = min(-0.4f, cosf(foo));
        //Vector3f LightDir(sinf(foo * 5.0f), y, cosf(foo * 5.0f));

        //  m_terrain.SetLightDir(LightDir);

        m_terrain.Render(*m_pGameCamera);
    }


    void PassiveMouseCB(int x, int y)
    {
        if (!m_showGui && !m_isPaused) {
            m_pGameCamera->OnMouse(x, y);
        }
    }

    void KeyboardCB(uint key, int state)
    {
        if (state == GLFW_PRESS) {

            switch (key) {

            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwDestroyWindow(window);
                glfwTerminate();
                exit(0);

            case GLFW_KEY_B:
                m_constrainCamera = !m_constrainCamera;
                printf("constrain %d\n", m_constrainCamera);
                break;

            case GLFW_KEY_C:
                m_pGameCamera->Print();
                break;

            case GLFW_KEY_W:
                //m_isWireframe = !m_isWireframe;

                //if (m_isWireframe) {
                //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                //}
                //else {
                //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                //}
                break;

            case GLFW_KEY_P:
                m_isPaused = !m_isPaused;

                if (m_isPaused == false)
                {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                else
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); 


                break;

            case GLFW_KEY_SPACE:
                m_showGui = !m_showGui;
                break;

            case GLFW_KEY_0:
                gShowPoints = 0;
                break;

            case GLFW_KEY_1:
                gShowPoints = 1;
                break;

            case GLFW_KEY_2:
                gShowPoints = 2;
                break;

            case GLFW_KEY_3:
                gShowPoints = 3;
                break;
            }
        }

        bool CameraChangedPos = m_pGameCamera->OnKeyboard(key);

        // Track if camera is moving
        if (state == GLFW_PRESS  || state == GLFW_REPEAT) {
            if (key == GLFW_KEY_W || key == GLFW_KEY_S ||
                key == GLFW_KEY_A || key == GLFW_KEY_D) {
                m_isMoving = true;
            }
        }
        else if (state == GLFW_RELEASE) {
            // Check if any movement key is still pressed
            if (key == GLFW_KEY_W || key == GLFW_KEY_S ||
                key == GLFW_KEY_A || key == GLFW_KEY_D) {
                m_isMoving = false;
            }
        }

        if (m_constrainCamera && CameraChangedPos) {
            ConstrainCameraToTerrain();
        }

        //bool CameraChangedPos = m_pGameCamera->OnKeyboard(key);

        //if (m_constrainCamera && CameraChangedPos) {
        //    ConstrainCameraToTerrain();
        //}
    }


    void MouseCB(int button, int action, int x, int y)
    {}


private:

    void CreateWindow_()
    {
        int major_ver = 0;
        int minor_ver = 0;
        bool is_full_screen = false;
        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 10");

        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }


    void InitCallbacks()
    {
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
    }


    void InitCamera()
    {
        float CameraX = m_terrain.GetWorldSize() / 2.0f;
        float CameraZ = CameraX;
        Vector3f Pos(CameraX, 0.0f, CameraZ);
        Pos = m_terrain.ConstrainCameraPosToTerrain(Pos);
        Vector3f Target(0.0f, 0.f, 1.0f);
        Vector3f Up(0.0, 1.0f, 0.0f);

        float FOV = 45.0f;
        float zNear = 0.01f;
        float zFar = Z_FAR;
        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
        m_pGameCamera->SetSpeed(0.05f);
    }

              
    void InitTerrain()
    {
        float WorldScale = 4.0f;
        float TextureScale = 16.0f;
        std::vector<string> TextureFilenames;
        TextureFilenames.push_back("assets/textures/rocky_trail_02_diff_1k.jpg");
        TextureFilenames.push_back("assets/textures/coast_sand_rocks_02_diff_2k.jpg");
        TextureFilenames.push_back("assets/textures/brown_mud_leaves_01_diff_2k.jpg");
        TextureFilenames.push_back("assets/textures/water.png");

        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames);

        m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);

        Vector3f LightDir(1.0f, -1.0f, 0.0f);

        m_terrain.SetLightDir(LightDir);

    }


    void InitGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 130";
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void ConstrainCameraToTerrain()
    {
        static const float m_cameraSmoothFactor = { 0.1f };  // Adjust between 0.01 (slow) to 1.0 (instant)

        Vector3f CurrentPos = m_pGameCamera->GetPos();
        Vector3f TargetPos = m_terrain.ConstrainCameraPosToTerrain(CurrentPos);

        // Smooth interpolation
        Vector3f NewPos;
        NewPos.x = CurrentPos.x + (TargetPos.x - CurrentPos.x) * m_cameraSmoothFactor;
        NewPos.y = CurrentPos.y + (TargetPos.y - CurrentPos.y) * m_cameraSmoothFactor;
        NewPos.z = CurrentPos.z + (TargetPos.z - CurrentPos.z) * m_cameraSmoothFactor;

        m_pGameCamera->SetPosition(NewPos);

        //Vector3f NewCameraPos = m_terrain.ConstrainCameraPosToTerrain(m_pGameCamera->GetPos());

        //m_pGameCamera->SetPosition(NewCameraPos);


        

    }


    GLFWwindow* window = NULL;
    BasicCamera* m_pGameCamera = NULL;
    bool m_isWireframe = false;
    MidpointDispTerrain m_terrain;
    bool m_showGui = false;
    bool m_isPaused = false;
    int m_terrainSize = 513;
    float m_roughness = 1.f;
    float m_minHeight = 0.0f;
    float m_maxHeight = 220.0f;
    int m_patchSize = 17;
    float m_counter = 0.0f;
    bool m_constrainCamera = false;


    float m_bobTimer = 0.0f;
    bool m_isMoving = false;
    float m_bobAmplitude = 0.02f;      // Vertical bob amount
    float m_bobFrequency = 10.0f;       // Bob speed
    float m_bobHorizontalAmount = 0.01f; // Side-to-side sway

    float m_currentBobIntensity = 0.0f;
    float m_targetBobIntensity = 0.0f;
    float m_bobIntensitySmoothFactor = 0.1f; // How quickly bob ramps up/down
};

TerrainDemo10* app = NULL;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    app->KeyboardCB(key, action);
}


static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
    app->PassiveMouseCB((int)x, (int)y);
}


static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
{
    double x, y;

    glfwGetCursorPos(window, &x, &y);

    app->MouseCB(Button, Action, (int)x, (int)y);
}


int main(int argc, char** argv)
{
#ifdef _WIN64
    g_seed = GetCurrentProcessId();
#else
    g_seed = getpid();
#endif
    printf("random seed %d\n", g_seed);

    SRANDOM;

    app = new TerrainDemo10();

    app->Init();

    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    app->Run();

    delete app;

    return 0;
}





// Frustrum culling



//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "demo_config.h"
//#include "texture_config.h"
//#include "midpoint_disp_terrain.h"
//
//
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//static int g_seed = 0;
//
//int gShowPoints;
//
//
//class TerrainDemo9
//{
//public:
//
//    TerrainDemo9()
//    {}
//
//    virtual ~TerrainDemo9()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                ImGui::Begin("Terrain Demo 5");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderFloat("Max height", &this->m_maxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Terrain roughness", &this->m_roughness, 0.0f, 5.0f);
//
//                static float Height0 = 64.0f;
//                static float Height1 = 128.0f;
//                static float Height2 = 192.0f;
//                static float Height3 = 256.0f;
//
//                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
//                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
//                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
//                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    SRANDOM;
//                    m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);
//                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//            }
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        static float foo = 0.0f;
//        foo += 0.002f;
//
//        /*  float S = (float)m_terrainSize;
//          float R = 2.5f * S;
//
//          Vector3f Pos(S + cosf(foo) * R, m_maxHeight + 250.0f, S + sinf(foo) * R);
//          m_pGameCamera->SetPosition(Pos);
//
//          Vector3f Center(S, Pos.y * 0.50f, S);
//          Vector3f Target = Center - Pos;
//          m_pGameCamera->SetTarget(Target);
//          m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);*/
//
//        float y = min(-0.4f, cosf(foo));
//        Vector3f LightDir(sinf(foo * 5.0f), y, cosf(foo * 5.0f));
//
//        m_terrain.SetLightDir(LightDir);
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui && !m_isPaused) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//                break;
//
//            case GLFW_KEY_P:
//                m_isPaused = !m_isPaused;
//                break;
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//
//            case GLFW_KEY_0:
//                gShowPoints = 0;
//                break;
//
//            case GLFW_KEY_1:
//                gShowPoints = 1;
//                break;
//
//            case GLFW_KEY_2:
//                gShowPoints = 2;
//                break;
//
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 9");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(0.0f, m_maxHeight + 100.0f, -150.0f);
//        Vector3f Target(0.0f, -0.25f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.01f;
//        float zFar = Z_FAR;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 2.0f;
//        float TextureScale = 4.0f;
//        std::vector<string> TextureFilenames;
//        TextureFilenames.push_back("assets/textures/IMGP5525_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/IMGP5487_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/tilable-IMG_0044-verydark.png");
//        TextureFilenames.push_back("assets/textures/water.png");
//
//        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames);
//
//        m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);
//
//        Vector3f LightDir(1.0f, -1.0f, 0.0f);
//
//        m_terrain.SetLightDir(LightDir);
//
//    }
//
//
//    void InitGUI()
//    {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    MidpointDispTerrain m_terrain;
//    bool m_showGui = false;
//    bool m_isPaused = false;
//    int m_terrainSize = 513;
//    float m_roughness = 1.0f;
//    float m_minHeight = 0.0f;
//    float m_maxHeight = 150.0f;
//    int m_patchSize = 33;
//    float m_counter = 0.0f;
//};
//
//TerrainDemo9* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//#ifdef _WIN64
//    g_seed = GetCurrentProcessId();
//#else
//    g_seed = getpid();
//#endif
//    printf("random seed %d\n", g_seed);
//
//    SRANDOM;
//
//    app = new TerrainDemo9();
//
//    app->Init();
//
//    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}
//
//




/*

        Copyright 2022 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Terrain Rendering - demo 5 - Terrain Lighting
*/
//
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "texture_config.h"
//#include "midpoint_disp_terrain.h"
//#include "demo_config.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//static int g_seed = 0;
//
//int gShowPoints;
//
//class TerrainDemo5
//{
//public:
//
//    TerrainDemo5()
//    {}
//
//    virtual ~TerrainDemo5()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                ImGui::Begin("Terrain Demo 5");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderFloat("Max height", &this->m_maxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Terrain roughness", &this->m_roughness, 0.0f, 5.0f);
//
//                static float Height0 = 64.0f;
//                static float Height1 = 128.0f;
//                static float Height2 = 192.0f;
//                static float Height3 = 256.0f;
//
//                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
//                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
//                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
//                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    SRANDOM;
//                    m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize,m_roughness, m_minHeight, m_maxHeight);
//                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//            }
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        //static float foo = 0.0f;
//        //foo += 0.002f;
//
//        /*  float S = (float)m_terrainSize;
//          float R = 2.5f * S;
//
//          Vector3f Pos(S + cosf(foo) * R, m_maxHeight + 250.0f, S + sinf(foo) * R);
//          m_pGameCamera->SetPosition(Pos);
//
//          Vector3f Center(S, Pos.y * 0.50f, S);
//          Vector3f Target = Center - Pos;
//          m_pGameCamera->SetTarget(Target);
//          m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);*/
//
//        //float y = min(-0.4f, cosf(foo));
//        //Vector3f LightDir(sinf(foo * 5.0f), y, cosf(foo * 5.0f));
//
//        //m_terrain.SetLightDir(LightDir);
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui && !m_isPaused) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//                break;
//
//            case GLFW_KEY_P:
//                m_isPaused = !m_isPaused;
//                break;
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//
//            case GLFW_KEY_0:
//                gShowPoints = 0;
//                break;
//
//            case GLFW_KEY_1:
//                gShowPoints = 1;
//                break;
//
//            case GLFW_KEY_2:
//                gShowPoints = 2;
//                break;
//
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 5");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(0.0f,m_maxHeight + 100.f, -150.0f);
//        Vector3f Target(0.0f, -0.25f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.01f;
//        float zFar = Z_FAR;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 2.0f;
//        float TextureScale = 4.0f;
//        std::vector<string> TextureFilenames;
//        TextureFilenames.push_back("assets/textures/IMGP5525_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/IMGP5487_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/tilable-IMG_0044-verydark.png");
//        TextureFilenames.push_back("assets/textures/water.png");
//
//        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames);
//
//        m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);
//
//        Vector3f LightDir(1.0f, -1.0f, 0.0f);
//
//        m_terrain.SetLightDir(LightDir);
//    }
//
//
//    void InitGUI()
//    {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    MidpointDispTerrain m_terrain;
//    bool m_showGui = false;
//    bool m_isPaused = false;
//    int m_terrainSize = 513;
//    int m_patchSize = 33;
//    float m_roughness = 1.0f;
//    float m_minHeight = 0.0f;
//    float m_maxHeight = 356.0f;
//    float m_counter = 0.0f;
//};
//
//TerrainDemo5* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//#ifdef _WIN64
//    g_seed = GetCurrentProcessId();
//#else
//    g_seed = getpid();
//#endif
//    printf("random seed %d\n", g_seed);
//
//    SRANDOM;
//
//    app = new TerrainDemo5();
//
//    app->Init();
//
//    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}




// Textured with lighting
//
//
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "texture_config.h"
//#include "midpoint_disp_terrain.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//static int g_seed = 0;
//
//
//class TerrainDemo5_1
//{
//public:
//
//    TerrainDemo5_1()
//    {}
//
//    virtual ~TerrainDemo5_1()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                ImGui::Begin("Terrain Demo 5");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderFloat("Max height", &this->m_maxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Terrain roughness", &this->m_roughness, 0.0f, 5.0f);
//                ImGui::SliderFloat("Light Softness", &this->m_lightSoftness, 0.0f, 50.0f);
//
//                static float Height0 = 64.0f;
//                static float Height1 = 128.0f;
//                static float Height2 = 192.0f;
//                static float Height3 = 256.0f;
//
//                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
//                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
//                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
//                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    SRANDOM;
//                    m_terrain.SetLight(m_lightDir, m_lightSoftness);
//                    m_terrain.CreateMidpointDisplacement(m_terrainSize, m_roughness, m_minHeight, m_maxHeight);
//                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//            }
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        /*     static float foo = 0.0f;
//             foo += 0.002f;
//
//             float S = (float)m_terrainSize;
//             float R = 2.5f * S;
//
//             Vector3f Pos(S + cosf(foo) * R, m_maxHeight + 350.0f, S + sinf(foo) * R);
//             m_pGameCamera->SetPosition(Pos);
//
//             Vector3f Center(S, Pos.y * 0.50f, S);
//             Vector3f Target = Center - Pos;
//             m_pGameCamera->SetTarget(Target);
//             m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);*/
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//                break;
//
//            case GLFW_KEY_P:
//                m_isPaused = !m_isPaused;
//                break;
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//
//            case GLFW_KEY_L:
//                m_terrain.Destroy();
//                SRANDOM;
//                m_counter += 0.1f;
//                m_lightDir.x = sinf(m_counter);
//                m_lightDir.z = cosf(m_counter);
//                m_terrain.SetLight(m_lightDir, m_lightSoftness);
//                m_terrain.CreateMidpointDisplacement(m_terrainSize, m_roughness, m_minHeight, m_maxHeight);
//                break;
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 5");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(545.0f, 550.0f, -600.0f);
//        Vector3f Target(-0.1f, -0.4f, 0.9f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.1f;
//        float zFar = 5000.0f;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 2.0f;
//        float TextureScale = 4.0f;
//        std::vector<string> TextureFilenames;
//        TextureFilenames.push_back("assets/textures/IMGP5525_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/IMGP5487_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/tilable-IMG_0044-verydark.png");
//        TextureFilenames.push_back("assets/textures/water.png");
//
//        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames, m_lightDir, m_lightSoftness);
//        m_terrain.CreateMidpointDisplacement(m_terrainSize, m_roughness, m_minHeight, m_maxHeight);
//    }
//
//
//    void InitGUI()
//    {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    MidpointDispTerrain m_terrain;
//    bool m_showGui = false;
//    bool m_isPaused = false;
//    int m_terrainSize = 512;
//    float m_roughness = 1.0f;
//    float m_lightSoftness = 4.0f;
//    float m_minHeight = 0.0f;
//    float m_maxHeight = 256.0f;
//    Vector3f m_lightDir = Vector3f(1.0f, -0.5f, 1.0f);
//    float m_counter = 0.0f;
//};
//
//TerrainDemo5_1* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//#ifdef _WIN64
//    g_seed = GetCurrentProcessId();
//#else
//    g_seed = getpid();
//#endif
//
//    // g_seed = 13788;
//    printf("random seed %d\n", g_seed);
//
//    SRANDOM;
//
//    app = new TerrainDemo5_1();
//
//    app->Init();
//
//    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}



// TEXTURED TERRAIN
//
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "texture_config.h"
//#include "midpoint_disp_terrain.h"
//#include "texture_generator.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//
//class TerrainDemo4
//{
//public:
//
//    TerrainDemo4()
//    {}
//
//    virtual ~TerrainDemo4()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                static int Iterations = 220;
//                static float MaxHeight = 320.f;
//                static float Roughness = 1.0f;
//
//                ImGui::Begin("Terrain Demo 4");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderInt("Iterations", &Iterations, 0, 1000);
//                ImGui::SliderFloat("MaxHeight", &MaxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Roughness", &Roughness, 0.0f, 5.0f);
//
//                static float Height0 = 64.0f;
//                static float Height1 = 128.0f;
//                static float Height2 = 192.0f;
//                static float Height3 = 256.0f;
//
//                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
//                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
//                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
//                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    int Size = 512;
//                    float MinHeight = 0.0f;
//                    m_terrain.CreateMidpointDisplacement(Size, Roughness, MinHeight, MaxHeight);
//                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//
//               
//
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//            }
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        static float foo = 0.0f;
//        float R = 1100.0f;
//        float S = 512.0f;
//
//        Vector3f Pos(S + cosf(foo) * R, 375.0f, S + sinf(foo) * R);
//        m_pGameCamera->SetPosition(Pos);
//
//        Vector3f Center(S, Pos.y * 0.60f, S);
//        Vector3f Target = Center - Pos;
//        m_pGameCamera->SetTarget(Target);
//        m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);
//
//        if (!m_isPaused) {
//            foo += 0.001f;
//        }
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//                break;
//
//            case GLFW_KEY_P:
//                m_isPaused = !m_isPaused;
//                break;
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 4");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(250.0f, 450.0f, -150.0f);
//        Vector3f Target(0.0f, -0.25f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.1f;
//        float zFar = 5000.0f;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//    //#define USE_TEXTURE_GENERATOR
//
//    void InitTerrain()
//    {
//#ifdef USE_TEXTURE_GENERATOR
//        InitTerrainTextureGenerator();
//#else
//        InitTerrainMultiTextures();
//#endif
//        m_terrain.SaveToFile("heightmap.png");
//    }
//
//
//    void InitTerrainTextureGenerator()
//    {
//        float WorldScale = 2.0f;
//        float TextureScale = 4.0f;
//
//        m_terrain.InitTerrain(WorldScale, TextureScale);
//
//        int Size = 512;
//        float Roughness = 1.0f;
//        float MinHeight = 0.0f;
//        float MaxHeight = 156.0f;
//
//        m_terrain.CreateMidpointDisplacement(Size, Roughness, MinHeight, MaxHeight);
//
//        TextureGenerator TexGen;
//
//        TexGen.LoadTile("assets/textures/rock02_2.jpg");
//        //TexGen.LoadTile("../Content/textures/IMGP5487_seamless.jpg");
//        //TexGen.LoadTile("../Content/textures/IMGP5525_seamless.jpg");
//        TexGen.LoadTile("assets/textures/rock01.jpg");
//
//        TexGen.LoadTile("assets/textures/tilable-IMG_0044-verydark.png");
//
//        // TexGen.LoadTile("../Content/textures/grass1.jpg");
//         //TexGen.LoadTile("../Content/textures/Rock6.png");
//
//        TexGen.LoadTile("assets/textures/water.png");
//        int TextureSize = 1024;
//
//        Texture* pTexture = TexGen.GenerateTexture(TextureSize, &m_terrain, MinHeight, MaxHeight);
//        m_terrain.SetTexture(pTexture);
//    }
//
//
//    void InitTerrainMultiTextures()
//    {
//        float WorldScale = 2.0f;
//
//        float TextureScale = 4.0f;
//
//        std::vector<string> TextureFilenames;
//        TextureFilenames.push_back("assets/textures/IMGP5525_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/IMGP5487_seamless.jpg");
//        TextureFilenames.push_back("assets/textures/tilable-IMG_0044-verydark.png");
//        TextureFilenames.push_back("assets/textures/water.png");
//
//        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames);
//
//        int Size = 512;
//        float Roughness = 1.f;
//        float MinHeight = 0.0f;
//        float MaxHeight = 300.f;
//
//        m_terrain.CreateMidpointDisplacement(Size, Roughness, MinHeight, MaxHeight);
//    }
//
//
//    void InitGUI()
//    {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    MidpointDispTerrain m_terrain;
//    bool m_showGui = false;
//    bool m_isPaused = false;
//};
//
//TerrainDemo4* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//    SRANDOM;
//
//    app = new TerrainDemo4();
//
//    app->Init();
//
//    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}




// TERRAIN 3 - midpoint displacement with heightmap

//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "midpoint_disp_terrain.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//
//class TerrainDemo3
//{
//public:
//
//    TerrainDemo3()
//    {}
//
//    virtual ~TerrainDemo3()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                static int Iterations = 100;
//                static float MaxHeight = 200.0f;
//                static float Roughness = 1.5f;
//
//                ImGui::Begin("Terrain Demo 3");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderInt("Iterations", &Iterations, 0, 1000);
//                ImGui::SliderFloat("MaxHeight", &MaxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Roughness", &Roughness, 0.0f, 5.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    int Size = 256;
//                    float MinHeight = 0.0f;
//                    m_terrain.CreateMidpointDisplacement(Size, Roughness, MinHeight, MaxHeight);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//            }
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        /*  static float foo = 0.0f;
//          float R = 400.0f;
//          float S = 128.0f;
//
//          Vector3f Pos(S + cosf(foo) * R, 250.0f, S + sinf(foo) * R);
//          m_pGameCamera->SetPosition(Pos);
//
//          Vector3f Center(128.0f, 50.0f, 128.0f);
//          Vector3f Target = Center - Pos;
//          m_pGameCamera->SetTarget(Target);
//          m_pGameCamera->SetUp(0.0f, 1.0f, 0.0f);
//
//          foo += 0.001f;*/
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 3");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(500.0f, 300.0f, -150.0f);
//        Vector3f Target(0.0f, -0.25f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.1f;
//        float zFar = 2000.0f;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 4.0f;
//        m_terrain.InitTerrain(WorldScale);
//
//        int Size = 256;
//        float Roughness = 1.0f;
//        float MinHeight = 0.0f;
//        float MaxHeight = 250.0f;
//
//        m_terrain.CreateMidpointDisplacement(Size, Roughness, MinHeight, MaxHeight);
//    }
//
//
//    void InitGUI()
//    {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    MidpointDispTerrain m_terrain;
//    bool m_showGui = false;
//};
//
//TerrainDemo3* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//    SRANDOM;
//
//    app = new TerrainDemo3();
//
//    app->Init();
//
//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}
//
//





//  TERRAIN 2 - FAULT FORMATION WITH HEIGHTMAP



//#ifdef _WIN64 // until I install imgui on Linux...
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//#endif
//
//#ifdef _WIN64
//#include <Windows.h>
//#endif
//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//
//#include "fault_formation_terrain.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//
//class TerrainDemo2
//{
//public:
//
//    TerrainDemo2()
//    {}
//
//    virtual ~TerrainDemo2()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow_(); // added '_' because of conflict with Windows.h
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//
//        InitGUI();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            glfwPollEvents();
//
//#ifdef _WIN64
//            if (m_showGui) {
//                // Start the Dear ImGui frame
//                ImGui_ImplOpenGL3_NewFrame();
//                ImGui_ImplGlfw_NewFrame();
//                ImGui::NewFrame();
//
//                static int Iterations = 100;
//                static float MaxHeight = 200.0f;
//                static float Filter = 0.2f;
//
//                ImGui::Begin("Terrain Demo 2");                          // Create a window called "Hello, world!" and append into it.
//
//                ImGui::SliderInt("Iterations", &Iterations, 0, 1000);
//                ImGui::SliderFloat("MaxHeight", &MaxHeight, 0.0f, 1000.0f);
//                ImGui::SliderFloat("Filter", &Filter, 0.0f, 1.0f);
//
//                if (ImGui::Button("Generate")) {
//                    m_terrain.Destroy();
//                    int Size = 256;
//                    float MinHeight = 0.0f;
//                    m_terrain.CreateFaultFormation(Size, Iterations, MinHeight, MaxHeight, Filter);
//                }
//
//                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//                ImGui::End();
//
//                // Rendering
//                ImGui::Render();
//                int display_w, display_h;
//                glfwGetFramebufferSize(window, &display_w, &display_h);
//                glViewport(0, 0, display_w, display_h);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//            }
//#endif
//
//            RenderScene();
//
//            glfwSwapBuffers(window);
//        }
//    }
//
//
//    void RenderScene()
//    {
//        if (!m_showGui) {
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        }
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        if (!m_showGui) {
//            m_pGameCamera->OnMouse(x, y);
//        }
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                }
//                else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//
//            case GLFW_KEY_SPACE:
//                m_showGui = !m_showGui;
//                break;
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {}
//
//
//private:
//
//    void CreateWindow_()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 2");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(200.0f, 400.0f, -150.0f);
//        Vector3f Target(0.0f, -0.35f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.1f;
//        float zFar = 2000.0f;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 4.0f;
//        m_terrain.InitTerrain(WorldScale);
//
//        int Size = 256;
//        int Iterations = 500;
//        float MinHeight = 0.0f;
//        float MaxHeight = 300.0f;
//        float Filter = 0.5;
//        m_terrain.CreateFaultFormation(Size, Iterations, MinHeight, MaxHeight, Filter);
//    }
//
//
//    void InitGUI()
//    {
//#ifdef _WIN64
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO(); (void)io;
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//
//        // Setup Dear ImGui style
//        ImGui::StyleColorsDark();
//
//        // Setup Platform/Renderer backends
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        const char* glsl_version = "#version 130";
//        ImGui_ImplOpenGL3_Init(glsl_version);
//#endif
//    }
//
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    FaultFormationTerrain m_terrain;
//    bool m_showGui = false;
//};
//
//TerrainDemo2* app = NULL;
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//    SRANDOM;
//
//    app = new TerrainDemo2();
//
//    app->Init();
//
//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}










//  TERRAIN 1


//
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <GL/glew.h>
//
//#include "ogldev_util.h"
//#include "ogldev_basic_glfw_camera.h"
//#include "ogldev_glfw.h"
//#include "terrain.h"
//
//#define WINDOW_WIDTH  1920
//#define WINDOW_HEIGHT 1080
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//static void CursorPosCallback(GLFWwindow* window, double x, double y);
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);
//
//
//class TerrainDemo1
//{
//public:
//
//    TerrainDemo1()
//    {
//    }
//
//    virtual ~TerrainDemo1()
//    {
//        SAFE_DELETE(m_pGameCamera);
//    }
//
//
//    void Init()
//    {
//        CreateWindow();
//
//        InitCallbacks();
//
//        InitCamera();
//
//        InitTerrain();
//    }
//
//
//    void Run()
//    {
//        while (!glfwWindowShouldClose(window)) {
//            RenderScene();
//            glfwSwapBuffers(window);
//            glfwPollEvents();
//        }
//    }
//
//
//    void RenderScene()
//    {
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        m_terrain.Render(*m_pGameCamera);
//    }
//
//
//    void PassiveMouseCB(int x, int y)
//    {
//        m_pGameCamera->OnMouse(x, y);
//    }
//
//    void KeyboardCB(uint key, int state)
//    {
//        if (state == GLFW_PRESS) {
//
//            switch (key) {
//
//            case GLFW_KEY_ESCAPE:
//            case GLFW_KEY_Q:
//                glfwDestroyWindow(window);
//                glfwTerminate();
//                exit(0);
//
//            case GLFW_KEY_C:
//                m_pGameCamera->Print();
//                break;
//
//            case GLFW_KEY_W:
//                m_isWireframe = !m_isWireframe;
//
//                if (m_isWireframe) {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                } else {
//                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                }
//
//                break;
//            }
//        }
//
//        m_pGameCamera->OnKeyboard(key);
//    }
//
//
//    void MouseCB(int button, int action, int x, int y)
//    {
//    }
//
//
//private:
//
//    void CreateWindow()
//    {
//        int major_ver = 0;
//        int minor_ver = 0;
//        bool is_full_screen = false;
//        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 1");
//
//        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
//    }
//
//
//    void InitCallbacks()
//    {
//        glfwSetKeyCallback(window, KeyCallback);
//        glfwSetCursorPosCallback(window, CursorPosCallback);
//        glfwSetMouseButtonCallback(window, MouseButtonCallback);
//    }
//
//
//    void InitCamera()
//    {
//        Vector3f Pos(100.0f, 220.0f, -400.0f);
//        Vector3f Target(0.0f, -0.25f, 1.0f);
//        Vector3f Up(0.0, 1.0f, 0.0f);
//
//        float FOV = 45.0f;
//        float zNear = 0.1f;
//        float zFar = 2000.0f;
//        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
//
//        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
//    }
//
//
//    void InitTerrain()
//    {
//        float WorldScale = 4.0f;
//        m_terrain.InitTerrain(WorldScale);
//#ifdef _WIN32		
//        m_terrain.LoadFromFile("assets\\heightmaps\\heightmap.save");
//#else 
//        m_terrain.LoadFromFile("assets/heightmaps/heightmap.save");
//#endif		
//    }
//
//    GLFWwindow* window = NULL;
//    BasicCamera* m_pGameCamera = NULL;
//    bool m_isWireframe = false;
//    BaseTerrain m_terrain;
//};
//
//TerrainDemo1* app = NULL;
//
//
//static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//    app->KeyboardCB(key, action);
//}
//
//
//static void CursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    app->PassiveMouseCB((int)x, (int)y);
//}
//
//
//static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
//{
//    double x, y;
//
//    glfwGetCursorPos(window, &x, &y);
//
//    app->MouseCB(Button, Action, (int)x, (int)y);
//}
//
//
//int main(int argc, char** argv)
//{
//    app = new TerrainDemo1();
//
//    app->Init();
//
//    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
//
//    app->Run();
//
//    delete app;
//
//    return 0;
//}
