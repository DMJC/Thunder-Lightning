#ifndef TANK_H
#define TANK_H

#include <landscape.h>
#include <modules/math/Vector.h>
#include <modules/math/Matrix.h>
#include <modules/model/model.h>
#include <modules/actors/simpleactor.h>
#include <modules/engines/tankengine.h>
#include <modules/engines/controls.h>

class TankBrain;
class SoundSource;

class Tank : virtual public SimpleActor, virtual public SigObject {
public:
    Tank(Ptr<IGame> thegame);

    virtual void action();

    virtual void draw();

    virtual void hitTarget(float damage);
    
    virtual void setLocation(const Vector & p);

    void explode();

    void shoot();

private:
    JRenderer * renderer;
    Ptr<ITerrain> terrain;

    // 3d models
    Ptr<Model> base, turret, cannon;

    // sound sources
    Ptr<SoundSource> sound_low, sound_high;

    // Tank state
    Ptr<TankControls> tank_controls;
    Ptr<TankEngine> tank_engine;
    Ptr<TankBrain> brain;
    Ptr<IActor> target;
    float damage;
    double age;
};




#endif