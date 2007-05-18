#include <modules/engines/ObserverEngine.h>
#include <remap.h>
#include "Observer.h"

Observer::Observer(Ptr<IGame> thegame)
    : SimpleActor(thegame)
    , dollying(false)
    , panning(false)
{
    mapEvents();
    setControlMode(MANUAL);
    setEngine(new ObserverEngine(thegame));
}

void Observer::action() {
    Ptr<DataNode> c = getControls();
    Ptr<EventRemapper> e = thegame->getEventRemapper();
    
    c->setFloat("accel_x", 0);
    c->setFloat("accel_y", 0);
    c->setFloat("accel_z", 0);
    c->setFloat("rotate_x", 0);
    c->setFloat("rotate_y", 0);
    c->setFloat("rotate_z", 0);

    //FIXME: Should move constants to config
    //FIXME: Should not use mouse_rel_* but proper axes to allow configurability
    //FIXME: Create a proper axis for mouse speed
    float delta_t = 20*thegame->getClock()->getRealFrameDelta();
    
    if (isDollying()) {
        c->setFloat("accel_z", -15*e->getAxis("mouse_rel_y") / delta_t);
    } else if (isPanning()) {
        c->setFloat("accel_x", 10*e->getAxis("mouse_rel_x") / delta_t);
        c->setFloat("accel_y", -10*e->getAxis("mouse_rel_y") / delta_t);
    } else {
        c->setFloat("rotate_y", 0.2f*e->getAxis("mouse_rel_x") / delta_t);
        c->setFloat("rotate_x", 0.2f*e->getAxis("mouse_rel_y") / delta_t);
    }
    
    SimpleActor::action();
}

void Observer::draw() {
    if(thegame->getClock()->isPaused()) {
        // trigger action while paused so the observer can be moved in pause mode
        action();
    }
    
    // nothing to draw
}

void Observer::mapEvents() {
    Ptr<EventSheet> sheet = getEventSheet();
    sheet->map("+primary", SigC::bind(SigC::slot(*this, &Observer::setDollying), true));
    sheet->map("-primary", SigC::bind(SigC::slot(*this, &Observer::setDollying), false));
    sheet->map("+secondary", SigC::bind(SigC::slot(*this, &Observer::setPanning), true));
    sheet->map("-secondary", SigC::bind(SigC::slot(*this, &Observer::setPanning), false));
    sheet->map("+tertiary", SigC::slot(*this, &Observer::stop));
}

void Observer::setDollying(bool b) {
    dollying = b;
    if (!b) {
        //getControls()->setBool("stop_z", true);
    }
}

void Observer::setPanning(bool b) {
    panning = b;
    if (!b) {
        getControls()->setBool("stop_x", true);
        getControls()->setBool("stop_y", true);
    }
}

void Observer::stop() {
    getControls()->setBool("stop_z", true);
    getControls()->setBool("stop_x", true);
    getControls()->setBool("stop_y", true);
}

