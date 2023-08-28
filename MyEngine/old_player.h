#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <typeinfo>
#include <memory>

#include "typedefs.h"
#include "ECS.h"

using namespace glm;

class old_Player : public Entity {


public:
    std::string GetEditorName() override { // deprecate
        return "Player";
    };

    uint32_t getBehaviorHash() override {
        return cHash("Player");
    };


    float moveSpeed = 300;
    float airSpeed = 100;
    float jumpForce = 200;
    bool onGround = true;
    int onWall = 0;
    float maxGroundSpeed = 3;
    float maxAirSpeed = 9;
    vec2 spawnPos = vec2(0.0f);
    
    ColorRenderer* renderer;
    Rigidbody* rigidbody;

    void Start() override {

        renderer = getComponent<ColorRenderer>();
        rigidbody = getComponent<Rigidbody>();

        rigidbody->SetFixedRotation(true);
        reset();
    };
    void Update() override {

        vec2 vel = rigidbody->GetLinearVelocity();

        //left and right movement
        bool leftOrRight = false;
        if (input->getKey(KeyCode::LeftArrow) && vel.x > (onGround ? -maxGroundSpeed : -maxAirSpeed)) {
            rigidbody->AddForce(vec2(onGround ? -moveSpeed : -airSpeed, 0.0f) * DeltaTime);
            leftOrRight = true;
        }
        if (input->getKey(KeyCode::RightArrow) && vel.x < (onGround ? maxGroundSpeed : maxAirSpeed)) {
            rigidbody->AddForce(vec2(onGround ? moveSpeed : airSpeed, 0.0f) * DeltaTime);
            leftOrRight = true;
        }

        ////exerise finer control over deacceleration on the ground
        //if (!leftOrRight && onGround) {
        //    rigidbody->AddForce(vec2(-vel.x * 3, 0));
        //}

        if (input->getKeyDown(KeyCode::UpArrow)) {
            if (onGround)
                rigidbody->AddForce(vec2(0, jumpForce));

            /*else if (onWall != 0)
            {
                rigidbody.Velocity = new vec2(rigidbody.Velocity.x, 0);
                rigidbody.AddForce(new vec2(jumpForce * 0.7f * -onWall, jumpForce * 1.2f));
            }*/
        }

        //jumping off ground and walls
        /*if (Canvas.GetSpecialKeyDown(SpecialKeys.UP))
        {
            if (onGround)
                rigidbody.AddForce(new vec2(0, jumpForce));
            else if (onWall != 0)
            {
                rigidbody.Velocity = new vec2(rigidbody.Velocity.x, 0);
                rigidbody.AddForce(new vec2(jumpForce * 0.7f * -onWall, jumpForce * 1.2f));
            }
        }*/

        ////allows for variable jump height based on how long the jump button is held down
        //if ((!Canvas.GetSpecialKey(SpecialKeys.UP) || rigidbody.Velocity.y < 0) && !onGround && onWall == 0 && rigidbody.Velocity.y > -2)
        //    rigidbody.AddForce(new vec2(0, -25));

        ////ground detection via raycast
        //onGround = (Canvas.RayCast(transform.Position, transform.Position - new vec2(-46, 47)) ||
        //    Canvas.RayCast(transform.Position, transform.Position - new vec2(46, 47)));

        ////wall detection via raycast
        //if (Canvas.RayCast(transform.Position, transform.Position + new vec2(47, 45)) || Canvas.RayCast(transform.Position, transform.Position + new vec2(47, -45)))
        //    onWall = 1;
        //else if (Canvas.RayCast(transform.Position, transform.Position + new vec2(-47, 45)) || Canvas.RayCast(transform.Position, transform.Position + new vec2(-47, -45)))
        //    onWall = -1;
        //else
        //    onWall = 0;

        ////add slight magnetism to walls to prevent bouncing off and increase friction
        //if (onWall != 0)
        //    rigidbody.AddForce(new vec2(onWall * 8.0f, 0));

        ////color indicator for player state
        //guy.Tint = onGround ? new Color(0, 255, 0, 100) : onWall != 0 ? new Color(0, 0, 255, 100) : Color.Invisible;

        ////reset position
        //if (Canvas.GetKeyDown('r') || transform.Position.y < -800)
        //    reset();

        ////camera movement
        //Canvas.CameraPosition = vec2.Lerp(Canvas.CameraPosition, transform.Position + new vec2(0, 100), 0.8f);
    };



private:

    void reset() {
        rigidbody->SetPosition(spawnPos);
    };
};
