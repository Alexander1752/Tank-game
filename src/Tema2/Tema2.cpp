#include "Tema2.h"

#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <direct.h>

#include <cfloat>
#include <glm/gtx/string_cast.hpp>
#include <sstream>
#include <iomanip>

#define MAX_SCALE 2
#define MIN_SCALE .5

#define TURRET_SPEED .005f
#define CAMERA_SPEED .001f
#define DIST_SPEED 5.f // speed with which one moves closer/farther from tank

#define FIELD_AREA 100.f

#define FONT_SIZE 40

#define GAME_DURATION 120.f;
#define EXIT_TIMER 3.f

using namespace std;
using namespace m1;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */

Tema2* world;

BoxProps getRandomBuilding() {
    float w = 3 + 7.f * rand() / RAND_MAX;
    float h = 5 + 15.f * rand() / RAND_MAX;
    float d = 3 + 7.f * rand() / RAND_MAX;
    float angle = 2.f * glm::pi<float>() * rand() / RAND_MAX;
    float x = FIELD_AREA * rand() / RAND_MAX - FIELD_AREA / 2;
    float z = FIELD_AREA * rand() / RAND_MAX - FIELD_AREA / 2;
    
    return BoxProps(glm::vec3(x, h / 2, z), w, h, d, angle);
}

Tema2::Tank getRandomTank() {
    float x = FIELD_AREA * rand() / RAND_MAX - FIELD_AREA / 2;
    float z = FIELD_AREA * rand() / RAND_MAX - FIELD_AREA / 2;
    float angle = 2.f * glm::pi<float>() * rand() / RAND_MAX;
    return Tema2::Tank(world, glm::vec3(x, 0, z), angle);
}

Tema2::Tema2()
{
    txr = NULL;
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
    srand(time(NULL));

    world = this;

    fov = 60;
    zNear = 0.01f;
    zFar = 200.f;

    scale = .01f;
    destruction = 0;
    timer = GAME_DURATION;
    exitTimer = EXIT_TIMER;

    camera = new Tema2Camera::Camera();
    camera->Set(glm::vec3(0, 6.f, -8.f), glm::vec3(0, 1.5f, 0), glm::vec3(0, 1, 0));
    camera->projectionMatrix = glm::perspective(RADIANS(fov), window->props.aspectRatio, zNear, zFar);

    ortho = new Tema2Camera::Camera();
    ortho->Set(glm::vec3(0, 30.f, 0.f), glm::vec3(0, 1.5f, 0), glm::vec3(0, 0, 1));
    glm::vec2 res = glm::vec2(window->GetResolution());
    ortho->projectionMatrix = glm::ortho(-res.x * scale, res.x * scale, -res.y * scale, res.y * scale, zNear, zFar);


    // Sets the resolution of the small viewport
    miniMap = ViewportArea(res.x * .8 - 50, 50, res.x / 5.f, res.y / 5.f);
    wholeScreen = ViewportArea(0, 0, res.x, res.y);

    GetCameraInput()->SetActive(false);
    window->DisablePointer();
    ToggleGroundPlane();

    {
        Shader* shader = new Shader("Tema2shader");
        shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::TEMA2, "shaders", "VertexShader.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::TEMA2, "shaders", "FragmentShader.glsl"), GL_FRAGMENT_SHADER);
        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }

    {
        Mesh* mesh = new Mesh("plane");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "plane50.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("box");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "cube.3mf");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("sphere");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "sphere.3mf");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* track = new Mesh("track");
        track->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "track.3mf");
        meshes[track->GetMeshID()] = track;
    }

    {
        Mesh* body = new Mesh("body");
        body->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "body.3mf");
        meshes[body->GetMeshID()] = body;
    }

    {
        Mesh* turret = new Mesh("turret");
        turret->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "turret.3mf");
        meshes[turret->GetMeshID()] = turret;
    }

    {
        Mesh* barrel = new Mesh("barrel");
        barrel->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "tank"), "barrel.3mf");
        meshes[barrel->GetMeshID()] = barrel;
    }

    myTank = new Tank(this);

    int noBuildings = rand() % 21 + 10;

    while (noBuildings) {
        BoxProps b = getRandomBuilding();

        if (myTank->collides(b))
            continue;

        buildings.push_back(b);
        --noBuildings;
    }

    //buildings.push_back(BoxProps(glm::vec3(50, 1, 0), 10, 2, 500, 0));

    int noTanks = rand() % 3 + 2;

    while (noTanks) {
        Tank t = getRandomTank();

        for (auto& b : buildings)
            if (t.collides(b))
                continue;

        for (auto& tank : tanks)
            if (t.collides(tank))
                continue;

        if (myTank->collides(t))
            continue;

        tanks.push_back(t);
        --noTanks;
    }

    txr = new gfxc::TextRenderer(".", res.x, res.y);
    txr->Load("assets\\fonts\\Hack-Bold.ttf", FONT_SIZE);

    lights.push_back(light_source(glm::vec3(0, 20, 0), glm::vec3(0, .8, 0), glm::vec3(0, -1, 0), RADIANS(180), 0));
}

void SetViewportArea(const Tema2::ViewportArea& viewSpace, glm::vec3 colorColor)
{
    glViewport(viewSpace.x, viewSpace.y, viewSpace.width, viewSpace.height);

    glEnable(GL_SCISSOR_TEST);
    glScissor(viewSpace.x, viewSpace.y, viewSpace.width, viewSpace.height);

    glClearColor(colorColor.r, colorColor.g, colorColor.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void Tema2::FrameStart()
{
}


void Tema2::Update(float deltaTimeSeconds)
{
    static stringstream ss;
    timer -= deltaTimeSeconds;

    if (timer < 0 || myTank->lives <= 0) {
        timer = 0;
        exitTimer -= deltaTimeSeconds;

        if (exitTimer < 0) {
            ss.str("");
            ss << "Final score: " << myTank->score;

            int len = ss.str().length();
            cout << "\n\n" << setw(len) << setfill('*') << "" << '\n' << ss.str() << '\n' << setw(len) << setfill('*') << "" << "\n\n";

            this->Exit();
        }

    }

    SetViewportArea(wholeScreen, glm::vec3(0));

    for (auto& tank : tanks)
        myTank->updateCollision(tank);

    for (auto& b : buildings) {
        myTank->updateCollision(b);

        for (auto& t : tanks)
            t.updateCollision(b);
    }

    list<Tank>::iterator i = tanks.begin(), j, end = tanks.end();
    while (i != end)
    {
        j = i;
        j++;

        while (j != end) {
            i->updateCollision(*j);
            j++;
        }

        i++;
    }

    currentCam = camera;
    RenderScene(deltaTimeSeconds);

    ss.str("");
    ss << "Score: " << myTank->score;
    txr->RenderText(ss.str(), 0, 10, 1, timer > 0 && myTank->lives > 0 ? glm::vec3(1) : glm::vec3(1, 0, 0));

    ss.str("");
    ss << setw(2) << setfill('0') << int(timer / 60) << ':' << setw(2) << setfill('0') << int(timer) % 60;
    txr->RenderText(ss.str(), 0, 10 + FONT_SIZE, 1, timer > 0 && myTank->lives > 0 ? glm::vec3(1) : glm::vec3(1, 0, 0));

    SetViewportArea(miniMap, glm::vec3(0));

    // TODO(student): render the scene again, in the new viewport
    currentCam = ortho;
    RenderScene(0);

    if (myTank->cooldown > 0) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1), ortho->position - myTank->forward * (miniMap.height * scale * 5));
        modelMatrix = glm::rotate(modelMatrix, myTank->angle, glm::vec3(0, 1, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(myTank->cooldown / COOLDOWN * miniMap.width * 10 * scale, 10, scale / .005f)); 
        RenderSimpleMesh(meshes["box"], shaders["Tema2shader"], modelMatrix, glm::vec3(1, 0, 0));
    }

}

void Tema2::RenderScene(float deltaTimeSeconds)
{
    {
        glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(10, 1, 10));
        RenderSimpleMesh(meshes["plane"], shaders["Tema2shader"], modelMatrix, currentCam == ortho ? UIcolor : glm::vec3(.7, 1, .7));
    }

    /*glm::mat4 modelMatrix = glm::translate(glm::mat4(1), glm::vec3(-3, 3, 10));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(3));
    RenderSimpleMesh(meshes["sphere"], shaders["Tema2shader"], modelMatrix, collides(myTank->getTankBox(), glm::vec3(-3, 3, 10), 3) ? glm::vec3(1, .1, .1) : glm::vec3(.2));*/

    myTank->updateAnimation(deltaTimeSeconds);
    updateAndErase(projectiles, deltaTimeSeconds);
    updateAndErase(tanks, deltaTimeSeconds);

    for (auto& b : buildings)
        RenderBox(b, glm::vec3(.5));
}

void Tema2::RenderBox(BoxProps box, glm::vec3 color) {
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1), box.position);
    modelMatrix = glm::rotate(modelMatrix, box.angle, glm::vec3(0, 1, 0));
    modelMatrix = glm::scale(modelMatrix, box.size);

    RenderSimpleMesh(meshes["box"], shaders["Tema2shader"], modelMatrix, color);
}

void Tema2::FrameEnd()
{
}

template <typename T>
void Tema2::updateAndErase(std::list<T>& elements, float deltaTimeSeconds) {
    list<T>::iterator iter = elements.begin();
    list<T>::iterator end = elements.end();

    while (iter != end)
    {
        if (!iter->updateAnimation(deltaTimeSeconds))
        {
            iter = elements.erase(iter);
        }
        else iter++;
    }
}

void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, const glm::vec3& color)
{
    int materialShininess = 30;
    float materialKd = .5f, materialKs = .5f;

    if (!mesh || !shader || !shader->GetProgramID())
        return;

    // Render an object using the specified shader and the specified position
    glUseProgram(shader->program);

    GLint location = glGetUniformLocation(shader->program, "object_color");
    glUniform3fv(location, 1, glm::value_ptr(color));

    location = glGetUniformLocation(shader->program, "destruction");
    glUniform1i(location, destruction);

    glm::vec3 eyePosition = modelMatrix * glm::vec4(currentCam->position, 1);
    int eye_position = glGetUniformLocation(shader->program, "eye_position");
    glUniform3f(eye_position, eyePosition.x, eyePosition.y, eyePosition.z);

    // Set material property uniforms (shininess, kd, ks, object color) 
    int material_shininess = glGetUniformLocation(shader->program, "material_shininess");
    glUniform1i(material_shininess, materialShininess);

    int material_kd = glGetUniformLocation(shader->program, "material_kd");
    glUniform1f(material_kd, materialKd);

    int material_ks = glGetUniformLocation(shader->program, "material_ks");
    glUniform1f(material_ks, materialKs);

    int object_color = glGetUniformLocation(shader->program, "object_color");
    glUniform3f(object_color, color.r, color.g, color.b);

    // TODO(student): Set any other shader uniforms that you need
    for (int i = 0; i < lights.size(); ++i)
    {
        string name = string("lights[") + to_string(i) + string("].position");
        GLuint location = glGetUniformLocation(shader->program, name.c_str());
        glUniform3fv(location, 1, glm::value_ptr(lights[i].position));

        name = string("lights[") + to_string(i) + string("].color");
        location = glGetUniformLocation(shader->program, name.c_str());
        glUniform3fv(location, 1, glm::value_ptr(lights[i].color));

        name = string("lights[") + to_string(i) + string("].direction");
        location = glGetUniformLocation(shader->program, name.c_str());
        glUniform3fv(location, 1, glm::value_ptr(lights[i].direction));

        name = string("lights[") + to_string(i) + string("].cutoff");
        location = glGetUniformLocation(shader->program, name.c_str());
        glUniform1f(location, lights[i].cutoff);

        name = string("lights[") + to_string(i) + string("].type");
        location = glGetUniformLocation(shader->program, name.c_str());
        glUniform1i(location, lights[i].type);
    }

    location = glGetUniformLocation(shader->program, "no_lights");
    glUniform1i(location, lights.size());



    // Bind model matrix
    GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
    glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // Bind view matrix
    glm::mat4 viewMatrix = currentCam->GetViewMatrix();
    int loc_view_matrix = glGetUniformLocation(shader->program, "View");
    glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    // Bind projection matrix
    int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
    glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(currentCam->projectionMatrix));

    // Draw the object
    glBindVertexArray(mesh->GetBuffers()->m_VAO);
    glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema2::OnInputUpdate(float deltaTime, int mods)
{
    // move the camera if M button is pressed
    if (window->KeyHold(GLFW_KEY_M))
    {
        float cameraSpeed = 2.0f;

        if (window->KeyHold(GLFW_KEY_W)) {
            // TODO(student): Translate the camera forward
            camera->MoveForward(deltaTime * cameraSpeed);
        }

        if (window->KeyHold(GLFW_KEY_A)) {
            // TODO(student): Translate the camera to the left
            camera->MoveRight(-deltaTime * cameraSpeed);
        }

        if (window->KeyHold(GLFW_KEY_S)) {
            // TODO(student): Translate the camera backward
            camera->MoveForward(-deltaTime * cameraSpeed);
        }

        if (window->KeyHold(GLFW_KEY_D)) {
            // TODO(student): Translate the camera to the right
            camera->MoveRight(deltaTime * cameraSpeed);
        }

        if (window->KeyHold(GLFW_KEY_F)) {
            // TODO(student): Translate the camera downward
            camera->MoveUpward(-deltaTime * cameraSpeed);
        }

        if (window->KeyHold(GLFW_KEY_R)) {
            // TODO(student): Translate the camera upward
            camera->MoveUpward(deltaTime * cameraSpeed);
        }

        return;
    }

    if (myTank->knockbackTimer <= 0 && myTank->lives > 0) {
        if (window->KeyHold(GLFW_KEY_W)) {
            // TODO(student): Translate the camera forward
            myTank->pos += myTank->forward * (deltaTime * FORWARD_SPEED);
            camera->position += myTank->forward * (deltaTime * FORWARD_SPEED);
            ortho->position += myTank->forward * (deltaTime * FORWARD_SPEED);

            myTank->fwd = true;
        }

        if (window->KeyHold(GLFW_KEY_A)) {
            // TODO(student): Translate the camera to the left
            myTank->updateAngle(deltaTime * ROTATE_SPEED);
            camera->RotateThirdPerson_OY(deltaTime * ROTATE_SPEED);
            ortho->RotateThirdPerson_OY(deltaTime * ROTATE_SPEED);

            myTank->spin = true;
        }

        if (window->KeyHold(GLFW_KEY_S)) {
            // TODO(student): Translate the camera backward
            myTank->pos -= myTank->forward * (deltaTime * FORWARD_SPEED);
            camera->position -= myTank->forward * (deltaTime * FORWARD_SPEED);
            ortho->position -= myTank->forward * (deltaTime * FORWARD_SPEED);

            myTank->fwd = true;
        }

        if (window->KeyHold(GLFW_KEY_D)) {
            // TODO(student): Translate the camera to the right
            myTank->updateAngle(-deltaTime * ROTATE_SPEED);
            camera->RotateThirdPerson_OY(-deltaTime * ROTATE_SPEED);
            ortho->RotateThirdPerson_OY(-deltaTime * ROTATE_SPEED);

            myTank->spin = true;
        }
    }

    // TODO(student): Change projection parameters. Declare any extra
    // variables you might need in the class header. Inspect this file
    // for any hardcoded projection arguments (can you find any?) and
    // replace them with those extra variables.

    if (mods & GLFW_MOD_CONTROL)
        deltaTime *= 5;

    if (window->KeyHold(GLFW_KEY_LEFT))
        fov = max(10.f, fov - deltaTime * 20.f);
    else if (window->KeyHold(GLFW_KEY_RIGHT))
        fov = min(180.f, fov + deltaTime * 20.f);
    else if (window->KeyHold(GLFW_KEY_UP)) {
        camera->distanceToTarget -= deltaTime * DIST_SPEED;
        camera->position += camera->forward * deltaTime * DIST_SPEED;
    }    
    else if (window->KeyHold(GLFW_KEY_DOWN)) {
        camera->distanceToTarget += deltaTime * DIST_SPEED;
        camera->position -= camera->forward * deltaTime * DIST_SPEED;
    }
    else if (window->KeyHold(GLFW_KEY_KP_6)) {
        scale = max(.005f, scale - deltaTime * .01f);
    }
    else if (window->KeyHold(GLFW_KEY_KP_4)) {
        scale += deltaTime * .01f;
    }
    else return;

    updatePerspectiveMatrix();
}

void Tema2::OnKeyPress(int key, int mods)
{
    // Add key press event
    if (key == GLFW_KEY_W || key == GLFW_KEY_S)
        myTank->fwd = true;

    if (key == GLFW_KEY_A || key == GLFW_KEY_D)
        myTank->spin = true;
}


void Tema2::OnKeyRelease(int key, int mods)
{
    // Add key release event
    if (key == GLFW_KEY_W || key == GLFW_KEY_S)
        myTank->fwd = false;

    if (key == GLFW_KEY_A || key == GLFW_KEY_D)
        myTank->spin = false;
}


void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
    if (window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
    {
        camera->RotateThirdPerson_OX(-deltaY * CAMERA_SPEED);
        camera->RotateThirdPerson_OY(-deltaX * CAMERA_SPEED);
    } else if (timer > 0 && myTank->lives > 0) {
        myTank->turretAngle -= deltaX * TURRET_SPEED;
    }
}


void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
    if (IS_BIT_SET(button, GLFW_MOUSE_BUTTON_LEFT) && myTank->lives > 0)
        myTank->fire();
}


void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
}


void Tema2::OnWindowResize(int width, int height)
{
    delete txr;
    txr = new gfxc::TextRenderer(".", width, height);
    txr->Load("assets\\fonts\\Hack-Bold.ttf", FONT_SIZE);

    miniMap = ViewportArea(width * .8 - 50, 50, width / 5.f, height / 5.f);
    wholeScreen = ViewportArea(0, 0, width, height);

    updatePerspectiveMatrix();
}

void Tema2::updatePerspectiveMatrix()
{
    glm::vec2 res = glm::vec2(window->GetResolution()) * scale;
    ortho->projectionMatrix = glm::ortho(-res.x, res.x, -res.y, res.y, zNear, zFar);
    
    camera->projectionMatrix = glm::perspective(RADIANS(fov), window->props.aspectRatio, zNear, zFar);
}

bool Tema2::collides(BoxProps b1, BoxProps b2) {
    if (glm::length(b1.size) + glm::length(b2.size) < 2.f * glm::length(b1.position - b2.position)) // traditional sphere collider, faster
        return false;

    return collides(b1.size * .5f, b2.size * .5f, b2.position - b1.position, b1.angle, b2.angle) &&
           collides(b2.size * .5f, b1.size * .5f, b1.position - b2.position, b2.angle, b1.angle);
}

bool Tema2::collides(glm::vec3 s1, glm::vec3 s2, glm::vec3 center, float angle1, float angle2) {
    if (s1.y < center.y - s2.y || center.y + s2.y < -s1.y) // no intersect on Y axis
        return false;

    glm::vec2 min1(-s1.x, -s1.z), max1(s1.x, s1.z);
    glm::vec2 min2(FLT_MAX, FLT_MAX), max2(-FLT_MAX, -FLT_MAX);

    glm::mat4 rot = glm::rotate(glm::mat4(1), -angle1, glm::vec3(0, 1, 0));
    rot = glm::translate(rot, center);
    rot = glm::rotate(rot, angle2, glm::vec3(0, 1, 0));
    glm::vec3 rotated;

    for (int i = -1; i <=1; i += 2)
        for (int j = -1; j <= 1; j += 2) {
            rotated = rot * glm::vec4(i * s2.x, 0, j * s2.z, 1);

            if (rotated.x < min2.x)
                min2.x = rotated.x;
            if (rotated.x > max2.x)
                max2.x = rotated.x;

            if (rotated.z < min2.y)
                min2.y = rotated.z;
            if (rotated.z > max2.y)
                max2.y = rotated.z;
        }

    return min1.x <= max2.x && max1.x >= min2.x && min1.y <= max2.y && max1.y >= min2.y;
}

bool Tema2::collides(BoxProps b, glm::vec3 center, float radius) {
    b.size *= .5f;
    center -= b.position;
    center = glm::rotate(glm::mat4(1), -b.angle, glm::vec3(0, 1, 0)) * glm::vec4(center, 1);

    return glm::length(glm::clamp(center, -b.size, b.size) - center) <= radius;
}
