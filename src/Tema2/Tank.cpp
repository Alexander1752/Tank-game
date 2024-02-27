#include "core/engine.h"
#include "utils/gl_utils.h"

#include "Tema2.h"
#include <iostream>

namespace m1
{
	Tema2::Tank::Tank(Tema2* worldPtr, glm::vec3 position, float a) {
		world = worldPtr;
		pos = position;
		angle = a;
        turretAngle = 0;
        type = ENEMY;
        lives = max_lives = 3;
        fwd = spin = false;
        cooldown = MATCH_START_COOLDOWN;
        knockbackTimer = -1;
        timer = -1;
        state = IDLE;
        direction = 0;
        score = 0;

        updateAngle(0);
	}

    Tema2::Tank::Tank(Tema2* worldPtr) : Tema2::Tank::Tank(worldPtr, glm::vec3(0), 0) {
        type = FRIEND;
        lives = max_lives = 9;
    }

    void Tema2::Tank::updateAngle(float deltaAngle) {
        angle += deltaAngle;
        forward = glm::vec3(glm::sin(angle), 0, glm::cos(angle));
    }

    glm::vec3 Tema2::Tank::getTurretForward() {
        return  glm::vec3(glm::sin(angle + turretAngle), 0, glm::cos(angle + turretAngle));
    }

    bool Tema2::Tank::updateAnimation(float deltaTimeSeconds) {
        if (world->currentCam == world->camera) {
            cooldown -= deltaTimeSeconds;

            if (knockbackTimer > 0) {
                knockbackTimer -= deltaTimeSeconds;
                float dist = (knockbackSpeed - .5f * ACC * deltaTimeSeconds) * deltaTimeSeconds;

                pos += knockbackVec * dist;
                if (this == world->myTank) {
                    world->camera->position += knockbackVec * dist;
                    world->ortho->position += knockbackVec * dist;
                }
            }

            if (this != world->myTank && lives > 0 && knockbackTimer <= 0) {
                timer -= deltaTimeSeconds;

                switch (state) {
                    case FORWARD:
                        pos += forward * (direction * deltaTimeSeconds * FORWARD_SPEED);
                        break;
                    case SPIN:
                        updateAngle(direction * deltaTimeSeconds * ROTATE_SPEED);
                        break;
                    default:
                        break;
                }

                if (timer <= 0) {
                    state = rand() % 3;

                    switch (state) {
                        case FORWARD: {
                            timer = 2.f + 2.f * rand() / RAND_MAX;
                            glm::vec3 posFWD = pos + forward * (FORWARD_SPEED * timer) - world->myTank->pos;
                            glm::vec3 posBACK = pos - forward * (FORWARD_SPEED * timer) - world->myTank->pos;

                            direction = glm::length(posFWD) < glm::length(posBACK) ? 1 : -1;

                            break;
                        }
                        case SPIN: {
                            float deltaAngle = glm::asinf(glm::cross(forward, glm::normalize(world->myTank->pos - pos)).y);

                            direction = deltaAngle > 0 ? 1 : -1;
                            deltaAngle = fmin(direction * deltaAngle, MAX_SPIN);
                            timer = deltaAngle / ROTATE_SPEED;

                            break;
                        }
                        default:
                            break;
                    }

                    if (rand() % 10 < 2) // 20% chance to move away
                        direction = -direction;
                }

                if (glm::length(pos - world->myTank->pos) <= ATTACK_DIST) {
                    float deltaAngle = glm::asinf(glm::cross(getTurretForward(), glm::normalize(world->myTank->pos - pos)).y);

                    if (fabs(deltaAngle) <= deltaTimeSeconds * ROTATE_SPEED) {
                        turretAngle += deltaAngle;
                        fire();
                    }
                    else
                        turretAngle += deltaTimeSeconds * ROTATE_SPEED * (deltaAngle > 0 ? 1 : -1);
                }
            }
        }

        RenderTank();
        return true;
    }

    void Tema2::Tank::RenderTank() {
        if (lives < 0)
            lives = 0;

        world->destruction = 3 - int(glm::ceilf(3.f * lives / max_lives));
        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, pos);
            modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 1, 0));
            modelMatrix = glm::translate(modelMatrix, glm::vec3(1.25f, 0, 0));
            world->RenderSimpleMesh(world->meshes["track"], world->shaders["Tema2shader"], modelMatrix, world->trackColor);
        }

        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, pos);
            modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 1, 0));
            modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.25f, 0, 0));
            world->RenderSimpleMesh(world->meshes["track"], world->shaders["Tema2shader"], modelMatrix, world->trackColor);
        }

        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, pos);
            modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0, 1, 0));
            world->RenderSimpleMesh(world->meshes["body"], world->shaders["Tema2shader"], modelMatrix, world->tankColor[type] * .8f);
        }

        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, pos);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(glm::sin(angle), 0, glm::cos(angle)) * .5f);
            modelMatrix = glm::rotate(modelMatrix, angle + turretAngle, glm::vec3(0, 1, 0));
            world->RenderSimpleMesh(world->meshes["turret"], world->shaders["Tema2shader"], modelMatrix, world->tankColor[type] * .5f);
        }

        {
            glm::mat4 modelMatrix = glm::mat4(1);
            modelMatrix = glm::translate(modelMatrix, pos);

            modelMatrix = glm::translate(modelMatrix, glm::vec3(glm::sin(angle), 0, glm::cos(angle)) * .5f);
            modelMatrix = glm::rotate(modelMatrix, angle + turretAngle, glm::vec3(0, 1, 0));
            modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 2.35 * (1 - .05f * world->destruction), 1.f));
            world->RenderSimpleMesh(world->meshes["barrel"], world->shaders["Tema2shader"], modelMatrix, world->trackColor);
        }

        world->destruction = 0;
    }

    BoxProps Tema2::Tank::getTankBox() {
        return BoxProps(pos + glm::vec3(0, 1, 0),
                        3.f, 2.f, 6.f,
                        angle);
    }

    BoxProps Tema2::Tank::getTurretBox() {
        return BoxProps(pos + glm::vec3(glm::sin(angle), 5.f, glm::cos(angle)) * .5f,
                        2.f, 1.f, 1.8f,
                        angle + turretAngle);
    }

    BoxProps Tema2::Tank::getBarrelBox() {
        return BoxProps(pos + glm::vec3(0, 2.35, 0.f) + glm::vec3(glm::sin(angle + turretAngle), 0, glm::cos(angle + turretAngle)) * (1 + 3.f / 2) + glm::vec3(glm::sin(angle), 0, glm::cos(angle)) * .5f,
                        0.5f, 0.5f, 3.f,
                        angle + turretAngle);
    }

    glm::vec3 Tema2::Tank::getBarrelEnd() {
        return pos + glm::vec3(0, 2.35 * (1 - .05f * (max_lives - lives) * 3 / max_lives), 0.f) + glm::vec3(glm::sin(angle + turretAngle), 0, glm::cos(angle + turretAngle)) * 4.f + glm::vec3(glm::sin(angle), 0, glm::cos(angle)) * .5f;
    }

    void Tema2::Tank::fire() {
        if (cooldown > 0)
            return;

        cooldown = COOLDOWN;
        world->projectiles.push_back(Projectile(world, this, getBarrelEnd(), getTurretForward()));
    }

    bool Tema2::Tank::collides(BoxProps& b) {
        if (world->collides(getTankBox(), b) || world->collides(getBarrelBox(), b))
            return true;

        return false;
    }

    void Tema2::Tank::updateCollision(BoxProps& b) {
        if (collides(b))
            registerCollision(b);
    }

    bool Tema2::Tank::collides(Tank& t) {
        if (world->collides(getTankBox(), t.getTankBox()) || world->collides(getBarrelBox(), t.getBarrelBox()) ||
            world->collides(getTurretBox(), t.getBarrelBox()) || world->collides(t.getTurretBox(), getBarrelBox()))
            return true;

        return false;
    }

    void Tema2::Tank::updateCollision(Tank& t) {
        if (collides(t)) {
            registerCollision(t.getTankBox(), t.spin, t.fwd);
            t.registerCollision(getTankBox(), spin, fwd);
        }
    }

    void Tema2::Tank::registerCollision(BoxProps b) {
        registerCollision(b, spin, fwd);
    }

    void Tema2::Tank::registerCollision(BoxProps b, bool &spinning, bool &forward) {
        knockbackVec = pos - b.position;
        knockbackVec.y = 0;

        pos += knockbackVec * .05f;
        if (this == world->myTank) {
            world->camera->position += knockbackVec * .05f;
            world->ortho->position += knockbackVec * .05f;
        }

        knockbackVec = glm::normalize(knockbackVec);
        knockbackSpeed = KNOCKBACK_SPEED;

        if (spinning) knockbackSpeed *= 1.1f;
        if (forward) knockbackSpeed *= 1.2f;

        spinning = forward = false;

        knockbackTimer = knockbackSpeed / ACC;
    }

















    Tema2::Projectile::Projectile(Tema2* worldPtr, Tank* source, glm::vec3 position, glm::vec3 forwardVec) {
        world = worldPtr;
        pos = position;
        forward = forwardVec;
        ttl = TIMEOUT;
        radius = .2;
        alive = true;
        src = source;
    }

    bool Tema2::Projectile::updateAnimation(float deltaTime) {
        if (world->currentCam == world->camera) {
            ttl -= deltaTime;

            if (ttl < 0) {
                //std::cout << "Sphere died\n";
                return false;
            }

            pos += forward * (deltaTime * PROJECTILE_SPEED);

            /// TODO check collisions
            for (auto& t : world->tanks)
                if (src != &t && (world->collides(t.getTurretBox(), pos, radius) || world->collides(t.getBarrelBox(), pos, radius))) {
                    t.lives--;
                    src->score += 100;
                    alive = false;
                }

            if (src != world->myTank && (world->collides(world->myTank->getTurretBox(), pos, radius) || world->collides(world->myTank->getBarrelBox(), pos, radius))) {
                world->myTank->lives--;
                src->score += 100;
                alive = false;
            }

            for (auto& b : world->buildings)
                if (world->collides(b, pos, radius))
                    return false;
        }

        RenderSphere();
        return alive;
    }

    void Tema2::Projectile::RenderSphere() {
        modelMatrix = glm::translate(glm::mat4(1), pos);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(.2));
        world->RenderSimpleMesh(world->meshes["sphere"], world->shaders["Tema2shader"], modelMatrix, glm::vec3(.1));
    }
}