#pragma once

#include "components/simple_scene.h"
#include "lab_camera.h"

#define PROJECTILE_SPEED 10.f
#define TIMEOUT 10.f
#define COOLDOWN 1.f
#define MATCH_START_COOLDOWN 5.f

#define KNOCKBACK_SPEED 3.f
#define ACC 9.8f

#define ATTACK_DIST 30.f
#define MAX_SPIN (glm::pi<float>() / 8)
#define ACCURACY_ANGLE 15 // in degrees, lower is more accurate

#define FORWARD_SPEED 10
#define ROTATE_SPEED 1

#include "components/text_renderer.h"

struct BoxProps {
    glm::vec3 position;
    glm::vec3 size;
    float angle;

    BoxProps(glm::vec3 pos, glm::vec3 s, float a) : position(pos), size(s), angle(a) {}
    BoxProps(glm::vec3 pos, float x, float y, float z, float a) : position(pos), size(glm::vec3(x, y, z)), angle(a) {}
};

namespace m1
{
    class Tema2 : public gfxc::SimpleScene
    {
     public:
        Tema2();
        ~Tema2();

        void Init() override;

        struct ViewportArea
        {
            ViewportArea() : x(0), y(0), width(1), height(1) {}
            ViewportArea(int x, int y, int width, int height)
                : x(x), y(y), width(width), height(height) {}
            int x;
            int y;
            int width;
            int height;
        };

        struct light_source
        {
            int type;
            glm::vec3 position;
            glm::vec3 color;
            glm::vec3 direction;
            float cutoff;

            light_source(glm::vec3 pos, glm::vec3 col, int t = 0) : position(pos), color(col), direction(glm::vec3(0)), cutoff(0), type(t) {};
            light_source(glm::vec3 pos, glm::vec3 col, glm::vec3 dir, float cut, int t = 1) : position(pos), color(col), direction(dir), cutoff(cut), type(t) {};
        };

        enum TankType {
            FRIEND = 0,
            ENEMY = 1,
        };

        enum TankStates {
            FORWARD,
            SPIN,
            IDLE
        };

        class Tank
        {
        public:
            glm::vec3 pos, forward;
            float angle, turretAngle;

            glm::mat4 modelMatrix;
            Tema2* world;
            int score;

            enum TankType type;
            int lives, max_lives;
            float cooldown;
            
            bool fwd, spin;
            glm::vec3 knockbackVec, knockbackPos;
            float knockbackSpeed, knockbackTimer;

            int state;
            int direction;
            float timer;

            Tank(Tema2* worldPtr);
            Tank(Tema2* worldPtr, glm::vec3 position, float a);

            void Tema2::Tank::updateAngle(float deltaAngle);
            bool Tema2::Tank::updateAnimation(float deltaTimeSeconds);
            glm::vec3 Tema2::Tank::getTurretForward();
            void Tema2::Tank::RenderTank();
            /*virtual bool update(float deltaTimeSeconds);
            bool isInHitbox(glm::ivec2 coords);
            bool isInHitbox(glm::vec2 coords, float diameter);*/
            BoxProps getTankBox();
            BoxProps getTurretBox();
            BoxProps getBarrelBox();
            glm::vec3 getBarrelEnd();

            bool collides(BoxProps& b);
            void updateCollision(BoxProps& b);
            bool collides(Tank& t);
            void updateCollision(Tank& t);
            void registerCollision(BoxProps b);
            void registerCollision(BoxProps b, bool &spinning, bool &forward);


            void fire();
        };

        class Projectile
        {
        public:
            glm::vec3 pos, forward;
            glm::mat4 modelMatrix;
            Tema2* world;
            float ttl, radius;
            bool alive;

            Tank* src;

            Projectile(Tema2* worldPtr, Tank* source, glm::vec3 position, glm::vec3 forward);

            bool updateAnimation(float deltaTime);
            void RenderSphere();
            /*virtual bool update(float deltaTimeSeconds);
            bool isInHitbox(glm::ivec2 coords);
            bool isInHitbox(glm::vec2 coords, float diameter);*/
        };

     private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;

        void updatePerspectiveMatrix();
        void Tema2::RenderScene(float deltaTimeSeconds);
        void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, const glm::vec3& color);
        void Tema2::RenderBox(BoxProps box, glm::vec3 color);

        bool collides(BoxProps b1, BoxProps b2);
        bool collides(glm::vec3 s1, glm::vec3 s2, glm::vec3 center, float angle1, float angle2);
        bool collides(BoxProps b, glm::vec3 center, float radius);

     protected:
        Tema2Camera::Camera *camera, *ortho, *currentCam;
        ViewportArea miniMap, wholeScreen;

        // TODO(student): If you need any other class variables, define them here.
        float fov, zNear, zFar;
        float scale;

        int destruction;
        float timer, exitTimer;

        static constexpr glm::vec3 trackColor = glm::vec3(.8);
        static constexpr glm::vec3 UIcolor = glm::vec3(.6, .9, .6);
        static constexpr glm::vec3 tankColor[] = { glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) };

        Tank* myTank;
        std::list<Projectile> projectiles;
        std::list<Tank> tanks;
        std::list<BoxProps> buildings;

        gfxc::TextRenderer* txr;

        std::vector<light_source> lights;

        template <typename T> void updateAndErase(std::list<T>& elements, float deltaTimeSeconds);
    };
}   // namespace m1
