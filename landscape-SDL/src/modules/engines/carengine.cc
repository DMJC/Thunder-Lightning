#include <modules/clock/clock.h>
#include <interfaces/ITerrain.h>

#include "carengine.h"


CarEngine::CarEngine(Ptr<IGame> game,
    Ptr<CarControls> controls,
    const CarParams & params)
    : thegame(game),
      controls(controls),
      CarParams(params)
{
    terrain = thegame->getTerrain();
    p = Vector(0,0,0);
    v = 0.0f;
    front = Vector(0,0,1);
    update();
}


void CarEngine::run()
{
    float delta_t = thegame->getClock()->getStepDelta();

    float force = controls->getThrottle() * max_accel;
    force -= controls->getBrake() * max_brake;
    force -= v * std::abs(v) * drag_coeff;
    force -= v * res_coeff;

    //ls_message("force: %f speed: %f\n", force, v);

    v += delta_t * force / mass;

    float rot = (controls->getSteer() * v * delta_t) * max_rot_per_meter;
    front *= RotateYMatrix<float>(rot);

    p += delta_t*(front*v);

    update();
}

Vector CarEngine::getLocation()    { return p; }
Vector CarEngine::getFrontVector() { return front; }
Vector CarEngine::getRightVector() { return right; }
Vector CarEngine::getUpVector()    { return up; }
void CarEngine::getOrientation(Vector * up, Vector * right, Vector * front)
{
    *up = this->up;
    *right = this->right;
    *front = this->front;
}

Vector CarEngine::getMovementVector() { return v*front; }

void CarEngine::setLocation(const Vector & pos) {
    p = pos;
    update();
}

void CarEngine::setOrientation(const Vector & up,
                               const Vector & right,
                               const Vector & front)
{
    this->front = front;
    update();
}

void CarEngine::setMovementVector(const Vector & vel) {
    v = front*vel;
}

void CarEngine::update() {
    for(int i=0; i<3; i++)
        tripod[i][1] = terrain->getHeightAt(p[0]+tripod[i][0], p[2]+tripod[i][2]);
    up = (tripod[1] - tripod[0]) % (tripod[2] - tripod[0]);
    up.normalize();
    right = up % front;
    right.normalize();
    front = right % up;
    front.normalize();

    p[1] = terrain->getHeightAt(p[0], p[2]);
}