#include <cmath>
#include <cstdio>
#include <modules/actors/projectiles/smartmissile.h>
#include <modules/actors/projectiles/bullet.h>
#include <modules/clock/clock.h>
#include <modules/ai/PathManager.h>
#include <interfaces/ITerrain.h>
#include <Faction.h>

#include "ai.h"
#include "drone.h"

float Personality::evaluate(const Rating & r) {
    float x = 0.0;
    float factor;

    // evaluate attack rating

    factor  = .6 * confidence;
    factor += .2 * obedience;
    factor += .2 * experience;
    factor += .0 * cautiousness;
    x += factor * r.attack;

    // evaluate defense rating

    factor  = .0 * confidence;
    factor += .3 * obedience;
    factor += .3 * experience;
    factor += .4 * cautiousness;
    x += factor * r.defense;

    // evaluate order rating

    factor  = .2 * confidence;
    factor += .5 * obedience;
    factor += .1 * experience;
    factor += .2 * cautiousness;
    x += factor * r.order;

    // evaluate opportunity rating

    factor  = .7 * confidence;
    factor += .0 * obedience;
    factor += .3 * experience;
    factor += .2 * cautiousness;
    x += factor * r.opportunity;

    // evaluate necessity rating

    factor  = .2 * confidence;
    factor += .2 * obedience;
    factor += .3 * experience;
    factor += .3 * cautiousness;
    x += factor * r.necessity;

    // evaluate danger rating

    factor  = .0 * confidence;
    factor += .2 * obedience;
    factor += - 0.2 * experience;
    factor += - 0.4 * cautiousness;
    x += factor * r.danger;

    // return evaluated rating
    return x;
}


#define RAND ((float) rand() / (float) RAND_MAX)
void Personality::randomize() {
    float x1 = RAND, x2 = 1.0 - x1;
    float x11 = x1 * RAND, x12 = x1 - x11;
    float x21 = x2 * RAND, x22 = x2 - x21;
    
    confidence = x11;
    obedience = x12;
    experience = x21;
    cautiousness = x22;
    ls_warning("Randomized personality: "
               "confidence = %f "
               "obedience = %f "
               "experience = %f "
               "cautiousness = %f\n",
               confidence, obedience, experience, cautiousness);
               
}

PatrolIdea::PatrolIdea(
	Context & ctx,
    const Vector & area,
    float radius)
:	Idea(ctx,"Patrol"), area(area), radius(radius)
{
    choosePath();
}

Rating PatrolIdea::rate() {
    Rating r;
    r.attack = 0.2;
    r.defense = 0.2;
    r.order = 0.2;
    r.opportunity = 0.0;
    r.necessity = 0.5;
    r.danger = 0.3;
    return r;
}

namespace {
    float segment_pos(const Vector & a, const Vector & b, const Vector & p) {
        Vector d = b-a;
        return (p-a)*d / d.lengthSquare();
    }
    bool in_segment(const Vector & a, const Vector & b, const Vector & p) {
        float pos = segment_pos(a,b,p);
        return pos >= 0.0f && pos <= 1.0f;
    }
    bool beyond_segment(const Vector & a, const Vector & b, const Vector & p) {
        float pos = segment_pos(a,b,p);
        return pos > 1.0f;
    }
    Vector segment_dist(const Vector & a, const Vector & b, const Vector & p) {
        Vector d = b-a;
        d.normalize();
        float pos = (p-a)*d;
        return (p-a) - pos*d;
    }
}

#define PI 3.14159265358979323846
#define NAV_DIST 100.0
// void PatrolIdea::realize() {
//     Vector p = fi->getCurrentLocation();
//     //ls_message("p: "); p.dump();
//     //ls_message("nav: "); navpoint.dump();
//     Vector navpoint = path.front();
//     if ((navpoint - Vector(p[0], navpoint[1], p[2])).length() < NAV_DIST) {
//         path.pop_front();
//         if (path.empty()) choosePath();
//         navpoint = path.front();
//     }
//     float course = atan2(navpoint[0]-p[0], navpoint[2]-p[2]);
//     if (course < 0) course += 2*PI;
//
//     ap->setMode( AP_COURSE_MASK | AP_HEIGHT_MASK | AP_SPEED_MASK );
//     ap->setTargetCourse(course);
//     ap->setTargetHeight(400.0);
//     ap->setTargetSpeed(400.0);
// }

void PatrolIdea::realize() {

    Vector p = ctx.fi->getCurrentLocation();
    p[1] = 0.0f;
    //ls_message("p: "); p.dump();
    //ls_message("nav: "); navpoint.dump();
    while (path.size() >= 2 && beyond_segment(path[0], path[1], p)) path.pop_front();
    while (path.size() >= 3 && in_segment(path[1], path[2], p)) path.pop_front();
    if (path.size()<=1) {
        path.clear();
        choosePath();
        return;
    }

    Vector a,b; // The two active segment vertices
    if (path.size()>=3) {
        float t = segment_pos(path[0], path[1], p);
        a = (1.0f-t) * path[0] + t*path[1];
        b = (1.0f-t) * path[1] + t*path[2];
    } else {
        a = path[0];
        b = path[1];
    }

    Vector dist = segment_dist(a, b, p);
    float t = segment_pos(a,b,p);

    float tan_alpha = 0.001f * dist.length();
    dist.normalize();

    Vector d = (b-a).normalize();
    Vector v = ctx.fi->getCurrentVelocity();
    if (d*v > 0.0f) tan_alpha += 0.0001f*(dist*v);
    d -= dist * tan_alpha;

    float course = atan2(d[0], d[2]);
    if (course < 0) course += 2*PI;

    ctx.ap->setMode( AP_COURSE_MASK | AP_ALTITUDE_MASK | AP_SPEED_MASK );
    ctx.ap->setTargetCourse(course);
    ctx.ap->setTargetAltitude(a[1]+t*(b[1]-a[1]));
    ctx.ap->setTargetSpeed(400.0);
}

void PatrolIdea::postpone() {
    ctx.ap->reset();
}

std::string PatrolIdea::info() {
    char buf[256];
    Vector p = ctx.fi->getCurrentLocation();
    float course = ctx.ap->getTargetCourse();
    course *= 180.0 / PI;
    float current_course = ctx.fi->getCurrentCourse() * 180.0 / PI;
    snprintf(buf, 256, "Patroling.\n"
            "position: %+5.0f %+5.0f %+5.0f\n"
            "course %3.0f current course %3.0f",
            p[0], p[1], p[2], course, current_course);
    char buf2[256];
    char *p1= buf2, *p2= buf;
    for(int i=0; i<std::min((int)path.size(),4); i++) {
        snprintf(p1, 256, "%s\n  wp %d: %+5.0f %+5.0f %+5.0f (%5.0f)",
                p2, i, path[i][0], path[i][1], path[i][2],
                (p-Vector(path[i][0], p[1], path[i][2])).length());
        std::swap(p1,p2);
    }
    return p2;
}

class PatrolPathEvaluator : public AI::PathEvaluator {
    Ptr<ITerrain> terrain;
public:
    PatrolPathEvaluator(Ptr<ITerrain> t) : terrain(t) { }
    virtual float cost(const Vector & a, const Vector & b) {
        Vector d = b-a;
        float h_a = terrain->getHeightAt(a[0],a[2]);
        float h_b = terrain->getHeightAt(b[0],b[2]);
        h_a = std::max(0.0f, h_a - 300.0f);
        h_b = std::max(0.0f, h_b - 300.0f);
        d[1] = h_b - h_a;
        d[1] *= 80.0f;
        return d.length();
    }
};

class PatrolPathEvaluator2 : public AI::PathEvaluator {
    Ptr<ITerrain> terrain;
    FlightInfo *fi;
public:
    PatrolPathEvaluator2(Ptr<ITerrain> t, FlightInfo *fi)
    : terrain(t), fi(fi) { }
    virtual float cost(const Vector & a, const Vector & b) {
        float h = 500.0f;
        Vector v1(a[0],h,a[2]);
        Vector v2(b[0],h,b[2]);
        return terrain->lineCollides(v1, v2, &v2)?-1.0f:(v2-v1).length();
    }
};


namespace {
void smoothen_path(std::deque<Vector> & path) {
    if (path.size() < 4) return;
    Vector v0,v1,v2;
    std::deque<Vector>::iterator it = path.begin();
    v0 = *it++;
    v1 = *it++;
    v2 = *it++;
    do {
        it[-2] = 0.5f*(v0+v2);
        v0 = v1;
        v1 = v2;
        v2 = *it;
    } while (it++ != path.end());
}
}

void PatrolIdea::choosePath() {
    float alpha = 2.0 * PI * RAND;
    float r = radius * RAND;
    Vector navpoint = area + r*Vector(cos(alpha), 0, sin(alpha));
    Vector dist = navpoint - ctx.fi->getCurrentLocation();
    dist[1] = 0;
    //AI::PathManager pm(500.0f, 500.0f);
    float res = dist.length() / 24.0f;
    res = std::max(500.0f, res);
    AI::PathManager pm(res, res );
    PatrolPathEvaluator pe(ctx.terrain);
    pm.findPath(ctx.fi->getCurrentLocation() + 5.0f*ctx.fi->getCurrentVelocity(), navpoint, &pe);
    pm.getPath(&path);
    path.push_front(ctx.fi->getCurrentLocation());
    path.front()[1] = ctx.terrain->getHeightAt(path.front()[0], path.front()[2]) + 400.0f;
    path.back()[1] = ctx.terrain->getHeightAt(path.back()[0], path.back()[2]) + 400.0f;
    for(int i=1; i<path.size()-1; i++) {
        path[i][1] = path[0][1]
            + ((float) i / (path.size()-1)) * (path.back()[1]-path[0][1]);
        if (path[i][1] - ctx.terrain->getHeightAt(path[i][0], path[i][2]) < 200.0f)
            path[i][1] = ctx.terrain->getHeightAt(path[i][0], path[i][2]) + 200.0f;
    }
    for(int i=0; i<1; i++) smoothen_path(path);
}



#define TIME_FOR_TARGET_SELECT 5.0
#define TIME_FOR_MISSILE 15.0
#define TIME_FOR_BULLET 0.1
AttackIdea::AttackIdea(Context & ctx)
: 	Idea(ctx,"Attack")
{
    last_target_select = RAND * TIME_FOR_TARGET_SELECT;
    last_missile = RAND * TIME_FOR_MISSILE;
    last_bullet = RAND * TIME_FOR_BULLET;
    delta_t = 0.0;
    p = ctx.actor->getLocation();
}

#define TARGET_SEEK_DISTANCE 12000.0
#define TARGET_SEEK_DISTANCE_SQUARE = (12000.0*12000.0)
#define MISSILE_SHOOT_ANGLE (45.0 * PI / 180.0)
#define BULLET_SHOOT_ANGLE (1.0 * PI / 180.0)
#define TARGET_SEEK_ANGLE (165.0 * PI / 180.0)
#define BULLET_SPEED 800.0f
Rating AttackIdea::rate() {
    Rating rating;
    
    delta_t = ctx.thegame->getTimeDelta()/1000.0;
    
    last_target_select += delta_t;
    last_missile += delta_t;
    last_bullet += delta_t;
    
    if (!target && canSelectTarget()) selectTarget();
    
    rating.attack=0;
    rating.defense=0;
    rating.order=0;
    rating.opportunity=0;
    rating.necessity=0;
    rating.danger=0;
    
    if (target) {
        rating.danger += 0.5;
        rating.attack += 1.0;
        rating.opportunity += 0.7;
        
        evaluateSituation();
        
        if (canFireMissile()) {
            rating.attack+=0.3;
            rating.opportunity+=0.4;
        }
        
        if (canFireBullet()) {
            rating.attack+=0.1;
            rating.opportunity+=0.2;
        }
    }
    
    return rating;
}

void AttackIdea::realize() {
    ctx.controls->setFirePrimary(false);
    if (target && target->getState() == IActor::DEAD) target=0;
    if (target) followTarget();

    if (canFireMissile()) fireMissile();
    else if (canFireBullet()) fireBullet();
}

void AttackIdea::postpone() {
    ctx.controls->setFirePrimary(false);
    ctx.ap->reset();
}

std::string AttackIdea::info() {
    if (target) {
        Vector target_pos = target->getLocation();
        Vector d = target_pos - p;
        Vector v = ctx.actor->getMovementVector();
        float dist = d.length();
        d.normalize();
        v.normalize();
        float angle = acos(d*v) * 180.0 / PI;
        float course = atan2(d[0], d[2]);
        float my_course = ctx.fi->getCurrentCourse() * 180.0 /PI;
        if (course < 0) course += 2.0*PI;
        course *= 180.0 / PI;
        //float pitch = asin( Vector(0,1,0)*d ) * 180.0 / PI;
        float pitch = ctx.ap->getTargetPitch() * 180.0 / PI;

        char buf[256];
        snprintf(buf, 256, "attacking %s %s\n"
                "target position %7.2f %7.2f %7.2f\n"
                "own position %7.2f %7.2f %7.2f\n"
                "own speed: %4.2f target speed: %4.2f\n"
                "course to target %5.2f? current course %5.2f\n"
                "angle %5.2f dist %7.2f\n"
                "pitch %5.2f\n",
                target->getTargetInfo()->getTargetClass().name.c_str(),
                target->getTargetInfo()->getTargetName().c_str(),
                target_pos[0], target_pos[1], target_pos[2],
                p[0], p[1], p[2],
                ctx.actor->getMovementVector().length(),
                ctx.ap->getTargetSpeed(),
                course, my_course, angle, dist, pitch);
        
        return buf;
    } else return "attacking";
}

void AttackIdea::evaluateSituation() {
    p = ctx.actor->getLocation();
    v = ctx.actor->getMovementVector();
    front = ctx.actor->getFrontVector();
    
    double delta_t = ctx.thegame->getClock()->getStepDelta();
    rendezvous.updateTarget(
            delta_t, target->getLocation(), target->getMovementVector());
    rendezvous.updateSource(p, v + BULLET_SPEED*front, Vector(0,0,0));
    target_rendezvous = rendezvous.calculate();

    Vector d = target_rendezvous - p;
    
    target_dist = d.length();
    d.normalize();
    target_angle = acos(d*front);
    if (target_dist > TARGET_SEEK_DISTANCE) {
        target = 0;
        //ls_message("lost the target!\n");
    }
}


bool AttackIdea::canFireMissile() {
    return target &&
            last_missile > TIME_FOR_MISSILE &&
            target_angle < MISSILE_SHOOT_ANGLE;
}

void AttackIdea::fireMissile() {
    if (!target) return;
    Ptr<SmartMissile> missile = new SmartMissile(&*ctx.thegame, target);
    missile->shoot(
            ctx.actor->getLocation(),
            ctx.actor->getMovementVector(),
            Vector(ctx.actor->getMovementVector()).normalize());
    ctx.thegame->addActor(missile);

    last_missile = 0;
}

bool AttackIdea::canFireBullet() {
    return target &&
            last_bullet > TIME_FOR_BULLET &&
            target_angle < BULLET_SHOOT_ANGLE;
}

void AttackIdea::fireBullet() {
    if (!target) return;
    ctx.controls->setFirePrimary(true);
    /*
    Ptr<Bullet> bullet = new Bullet(&*thegame);
    Vector v=source->getMovementVector();
    Vector d=source->getFrontVector();
    d.normalize();
    bullet->shoot(p, v + BULLET_SPEED*d, d);
    thegame->addActor(bullet);

    last_bullet = 0;
    */
}

bool AttackIdea::canSelectTarget() {
    return !target && last_target_select > TIME_FOR_TARGET_SELECT;
}

void AttackIdea::selectTarget() {
    typedef IActorStage::ActorList List;
    typedef List::const_iterator Iter;
    List list;
    ctx.thegame->queryActorsInSphere(
        list,
        ctx.actor->getLocation(),
        TARGET_SEEK_DISTANCE);
        
    float best_dist = 0;
    Ptr<IActor> best_target;

    Vector v = ctx.actor->getMovementVector();
    v.normalize();

    for (Iter i=list.begin(); i!=list.end(); i++) {
        Ptr<IActor> actor = *i;
        if (actor == ctx.actor) continue;
        if(! actor->getTargetInfo() ) continue;
        const TargetInfo::TargetClass & tclass =
            actor->getTargetInfo()->getTargetClass();
        if(!tclass.is_radar_detectable) continue;
        if(ctx.actor->getFaction()->getAttitudeTowards(actor->getFaction())
            != Faction::HOSTILE) continue;
        Vector d = p - actor->getLocation();
        float dist = d.length();
        float angle = acos(v*d/dist);
        if (angle > TARGET_SEEK_ANGLE) {
            continue;
        }

        if ( best_dist==0 || dist < best_dist ) {
            best_dist = dist;
            best_target = *i;
        }
    }

    target = best_target;
}

void AttackIdea::followTarget() {
    Vector up(0,1,0);
    Vector d = target_rendezvous - p;
    float dist = d.length();
    d.normalize();

    ctx.ap->setMode(AP_PITCH_MASK | AP_COURSE_MASK | AP_SPEED_MASK);

    float pitch = asin( up*d );
    ctx.ap->setTargetPitch(pitch);

    float course = atan2(d[0], d[2]);
    if (course < 0) course += 2.0*PI;
    
    ctx.ap->setTargetCourse(course);
    float speed = target->getMovementVector().length() + (dist - 800.0) * 5.0;
    speed = std::max(100.0f, speed);
    ctx.ap->setTargetSpeed(speed);
}


Rating EvadeTerrainIdea::rate() {
    Rating rating;
    
    float height = ctx.fi->getCurrentHeight();
    
    if (triggered && height > 200) triggered=false;
    
    if (ctx.fi->collisionWarning()) {
        rating.defense += 0.4;
        rating.danger += 0.9;
        rating.necessity += 0.7;
    }
    
    if (height < 150.0) {
        float x = height / 150.0;
        x *= x;
        x = 1.0-x;
        rating.necessity += 3.0*x;
        rating.danger += 3.0*x;
    }
    
    rating.necessity += triggered?1:0;
    
    return rating;
}

void EvadeTerrainIdea::realize() {
	triggered=true;
    if (!ctx.fi->collisionWarning()) {
        ctx.ap->setMode(AP_HEIGHT_MASK);
        ctx.ap->setTargetHeight(400.0);
    } else {
        ctx.ap->setMode(AP_PITCH_MASK | AP_ROLL_MASK);
        ctx.ap->setTargetPitch(70*PI/180);
        ctx.ap->setTargetRoll(0);
    }
    ctx.controls->setThrottle(1);
    ctx.controls->setRudder(0);
}

void EvadeTerrainIdea::postpone() {
    ctx.ap->reset();
}

std::string EvadeTerrainIdea::info() {
	std::string nfo;
    if (ctx.fi->collisionWarning()) return "Pulling up";
    else return "Evading terrain";
}


Rating CRIdea::rate() {
	Rating r;
	r.attack = 1;
	r.defense = 1;
	r.order = 1;
	r.necessity = 1;
	r.opportunity = 1;
	return r;
}

void CRIdea::realize() {
	start();
}

void CRIdea::postpone() {
	CoRoutine::interrupt();
}

std::string CRIdea::info() {
	return nfo;
	ctx.ap->reset();
}

void CRIdea::run() {
	for(;;) {
		nfo = "Stablilizing";
		ctx.ap->setMode(AP_ROLL_MASK|AP_PITCH_MASK|AP_SPEED_MASK);
		ctx.ap->setTargetRoll(0);
		ctx.ap->setTargetPitch(0);
		ctx.ap->setTargetSpeed(200);
		
		do {
			yield();
		} while (
			std::abs(ctx.fi->getCurrentRoll()) > 0.02
			|| std::abs(ctx.fi->getCurrentPitch()) > 0.05
			|| std::abs(ctx.fi->getCurrentSpeed()) < 195);
			
		nfo = "Rolling";
		ctx.ap->reset();
		ctx.controls->setAileron(1);
		do {
			yield();
		} while (std::abs(ctx.fi->getCurrentRoll()) < 1.7);
		do {
			yield();
		} while (std::abs(ctx.fi->getCurrentRoll()) > 0.5);
		
		nfo = "Stablilizing";
		ctx.ap->setMode(AP_ROLL_MASK|AP_PITCH_MASK|AP_SPEED_MASK);
		ctx.ap->setTargetRoll(0);
		ctx.ap->setTargetPitch(0);
		ctx.ap->setTargetSpeed(200);
		
		do {
			yield();
		} while (
			std::abs(ctx.fi->getCurrentRoll()) > 0.02
			|| std::abs(ctx.fi->getCurrentPitch()) > 0.05
			|| std::abs(ctx.fi->getCurrentSpeed()) < 195);
			
		nfo = "Looping";
		ctx.ap->reset();
		
		ctx.controls->setThrottle(1);
		ctx.controls->setAileronAndRudder(0);
		ctx.controls->setElevator(-1);
		
		do {
			yield();
		} while (ctx.actor->getUpVector()[1] > -0.01);
		do {
			yield();
		} while (ctx.actor->getUpVector()[1] < 0.5);
		
		nfo = "Gaining Height";
		ctx.ap->setMode(AP_HEIGHT_MASK|AP_SPEED_MASK);
		ctx.ap->setTargetHeight(1000);
		ctx.ap->setTargetSpeed(200);
		do {
			yield();
		} while (
			std::abs(ctx.fi->getCurrentHeight()) < 995
			|| std::abs(ctx.fi->getCurrentSpeed()) < 195);
		
	}
}


Rating Dogfight::rate() {
	Rating r;
	Ptr<IActor> tgt = target;
	if (!tgt || !targetInRange(tgt)) {
		tgt = selectNearestTargetInRange();
	}
	
	if (tgt) {
		float dist = (tgt->getLocation()-ctx.actor->getLocation()).length();
		float factor = dist/5000;
		factor = 1-factor*factor;
		r.attack = 1*factor;
		r.defense = 0.5*factor;
		r.danger = 0.7*factor;
		r.order = 0.3*factor;
		r.necessity = 1*factor;
		r.opportunity = 1*factor;
	}
	return r;
}


void Dogfight::realize() {
	start();
}

void Dogfight::postpone() {
	CoRoutine::interrupt();
	ctx.controls->setFirePrimary(false);
}

std::string Dogfight::info() {
	return nfo;
}

void Dogfight::run() {
	for(;;) {
		nfo="Dogfighting";
		if (target && !targetInRange(target)) target = 0;
		if (!target) {
			target = selectNearestTargetInRange();
		}
		if (!target) {
			yield();
			continue;
		}
		
		if (positionFavorable()) {
			aimAndShoot();
		} else {
			evade();
		}
		
		yield();
	}
}

bool Dogfight::targetInRange(Ptr<IActor> tgt) {
	return (tgt->getLocation()-ctx.actor->getLocation()).length() < 5000;
}

Ptr<IActor> Dogfight::selectNearestTargetInRange(float range) {
    typedef IActorStage::ActorList List;
    typedef List::const_iterator Iter;
    List list;
    ctx.thegame->queryActorsInSphere(
    	list, ctx.actor->getLocation(), range);
    Ptr<Faction> faction = ctx.actor->getFaction();
    
    Ptr<IActor> best_candidate;
    float best_dist=0;
    
    for(Iter i=list.begin(); i!=list.end(); ++i) {
    	Ptr<IActor> candidate = *i;
    	if (candidate == ctx.actor) continue;
        if(!candidate->getTargetInfo() ) continue;
        const TargetInfo::TargetClass & tclass =
            candidate->getTargetInfo()->getTargetClass();
        if(!tclass.is_radar_detectable) continue;
        if(faction->getAttitudeTowards(candidate->getFaction())
        	!= Faction::HOSTILE) continue;
        Vector d = ctx.actor->getLocation() - candidate->getLocation();
        float dist = d.length();
        
        if (best_dist==0 || dist < best_dist) {
        	best_candidate = candidate;
        	best_dist = dist;
        }
    }
    
    return best_candidate;
}	


bool Dogfight::positionFavorable() {
	Vector target_dir = target->getFrontVector();
	Vector p = ctx.actor->getLocation();
	Vector target_p = target->getLocation();
	float dist = (target_p-p).length();
	bool behind_target = ((target_p-p)*target_dir)>0;
	return dist > 160 && (behind_target || dist>2000);
}

void Dogfight::aimInDirection(Vector d) {
    float pitch = asin( d[1] );
    ctx.ap->setTargetPitch(pitch);

    float course = atan2(d[0], d[2]);
    if (course < 0) course += 2.0*PI;
    ctx.ap->setTargetCourse(course);
}

void Dogfight::aimAndShoot() {
	Rendezvous rv;
	rv.setVelocity(BULLET_SPEED+ctx.fi->getCurrentSpeed());
	char buf[1024];
	nfo = "aiming and shooting";
	ctx.ap->setMode(AP_SPEED_MASK|AP_COURSE_MASK|AP_PITCH_MASK);
	while (positionFavorable() && targetInRange(target)) {
		rv.updateSource(
			ctx.actor->getLocation(),
			Vector(0,0,0),
			Vector(0,-9.81,0));
		rv.updateTarget(
			ctx.thegame->getClock()->getStepDelta(),
			target->getLocation(),
			target->getMovementVector());
		Vector p = ctx.actor->getLocation();
		Vector target_p = rv.calculate();
		Vector target_v = target->getMovementVector();
		Vector dir_to_target = target_p-p;
		float dist = dir_to_target.length();
		dir_to_target.normalize();
		
		// we calculate the deviation between the front vector
		// (were the cannon is aimed at)and the actual
		// projectile direction, thus getting a corrected aiming
		// vector
		Vector front = ctx.actor->getFrontVector();
		Vector projectile_dir = ctx.actor->getMovementVector()
			+ BULLET_SPEED*front;
		projectile_dir.normalize();
		
		dir_to_target -= (projectile_dir - front);
		dir_to_target.normalize();
		
		snprintf(buf, 1024, "Aiming and shooting\n"
			"p: %5.2f %5.2f %5.2f\n"
			"target: %5.2f %5.2f %5.2f\n"
			"dir to target: %5.2f %5.2f %5.2f\n"
			"dir: %5.2f %5.2f %5.2f\n"
			"elevator: %1.3f\n"
			"aileron; %1.3f",
			p[0], p[1], p[2],
			target_p[0], target_p[1], target_p[2],
			dir_to_target[0], dir_to_target[1], dir_to_target[2],
			ctx.actor->getFrontVector()[0],ctx.actor->getFrontVector()[1],ctx.actor->getFrontVector()[2],
			ctx.controls->getElevator(),
			ctx.controls->getAileron());
		nfo = buf;
		aimInDirection(dir_to_target);
		
	    float speed_diff = (target_v-ctx.actor->getMovementVector())
	    	*ctx.actor->getFrontVector();
	    float speed = target_v.length() + (dist - 500.0) + 15*speed_diff;
	    
	    speed = std::max(100.0f, speed);
	    ctx.ap->setTargetSpeed(speed);
	    
	    float cos_error = dir_to_target*ctx.actor->getFrontVector();
	    float tan_error = sqrt(1-cos_error*cos_error)/cos_error;
	    
	    ctx.controls->setFirePrimary(tan_error <= 15/dist);
	    
	    yield();
	}
	ctx.ap->reset();
	ctx.controls->setFirePrimary(false);
}

void Dogfight::evade() {
	Vector p_me = ctx.actor->getLocation();
	Vector p_target = target->getLocation();
	Vector delta_p = p_target - p_me;
	Vector v_me = ctx.actor->getMovementVector();
	Vector v_target = target->getMovementVector();
	
	bool faster_than_enemy = 
		v_me.length() > v_target.length();
	bool behind_enemy = delta_p * v_target > 0;
	bool enemy_behind = delta_p * v_me < 0;
	bool high_altitude = ctx.fi->getCurrentHeight() > 1500;
	bool enemy_above = delta_p[1] > 0;
		
	if (behind_enemy) {
		if (enemy_behind) {
			gainSpeedAndStabilize();
			if (enemy_above || !high_altitude) {
				flyImmelmannTurn();
			} else {
				flySplitS();
			}
		} else if (faster_than_enemy) {
			float r = 3*RAND;
			if (r < 2)
				flyScissors();
			else {
				gainSpeedAndStabilize();
				flyLooping();
			}
		} else {
			gainSpeedAndStabilize();
		}
	} else {
		if (enemy_behind) {
			float r = 7*RAND;
			if (r<1) {
				gainSpeedAndStabilize();
				flyLooping();
			} else if (r<2) {
				flyScissors();
			} else if (r<3) {
				gainSpeedAndStabilize();
				flyImmelmann();
			} else if (r<4 || !high_altitude) {
				gainSpeedAndStabilize();
				flyImmelmannTurn();
			} else {
				stabilize();
				flySplitS();
			}
		}
	}
}

void Dogfight::gainSpeedAndStabilize() {
	nfo = "Gaining speed and stabilizing";
	ctx.controls->setAileronAndRudder(0);
	ctx.controls->setElevator(0);
	ctx.ap->setMode(AP_PITCH_MASK|AP_ROLL_MASK|AP_SPEED_MASK);
	ctx.ap->setTargetRoll(0);
	ctx.ap->setTargetPitch(0);
	ctx.ap->setTargetSpeed(200);
	do {
		yield();
		} while (
			std::abs(ctx.fi->getCurrentRoll()) > 0.02
			|| std::abs(ctx.fi->getCurrentPitch()) > 0.05
			|| std::abs(ctx.fi->getCurrentSpeed()) < 195);
	ctx.ap->reset();
}

void Dogfight::stabilize() {
	nfo = "Stabilizing";
	ctx.controls->setAileronAndRudder(0);
	ctx.controls->setElevator(0);
	ctx.ap->setMode(AP_PITCH_MASK|AP_ROLL_MASK);
	ctx.ap->setTargetRoll(0);
	ctx.ap->setTargetPitch(0);
	do {
		yield();
		} while (
			std::abs(ctx.fi->getCurrentRoll()) > 0.02
			|| std::abs(ctx.fi->getCurrentPitch()) > 0.05);
	ctx.ap->reset();
}

void Dogfight::flyLooping() {
	nfo = "Looping";
	ctx.controls->setAileronAndRudder(0);
	ctx.controls->setElevator(-1);
	
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] > -0.01);
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] < 0.5);
}

void Dogfight::flyScissors() {
	nfo="Scissors";
	char buf[1024];
	
	ctx.ap->setMode(AP_HEIGHT_MASK|AP_COURSE_MASK);
	ctx.controls->setThrottle(1);
	ctx.ap->setTargetHeight(ctx.fi->getCurrentHeight());
	
	Vector d = ctx.actor->getFrontVector();
	d[1] = 0;
	d.normalize();
    float course = atan2(d[0], d[2]);
    float course_left = course - 0.8;
    float course_right = course + 0.8;
    if (course_left < 0) course_left += 2*PI;
    if (course_right >= 2*PI) course_right -= 2*PI;
	
	ctx.ap->setTargetCourse(course_left);
	float error;
	do {
		yield();
		d = ctx.actor->getFrontVector();
		d[1] = 0;
		d.normalize();
	    float course = atan2(d[0], d[2]);
	    error = course_left - course;
	    if (error < -PI) error += 2*PI;
	    if (error > PI) error -= 2*PI;
	} while (error < 0);
	ctx.ap->setTargetCourse(course_right);
	do {
		yield();
		d = ctx.actor->getFrontVector();
		d[1] = 0;
		d.normalize();
	    float course = atan2(d[0], d[2]);
	    error = course_right - course;
	    if (error < -PI) error += 2*PI;
	    if (error > PI) error -= 2*PI;
	} while (error > 0);
    
	ctx.ap->reset();
}


void Dogfight::flyImmelmann() {
	nfo = "Immelmann pulling up.";
	ctx.ap->setMode(AP_PITCH_MASK|AP_ROLL_MASK);
	ctx.ap->setTargetRoll(0);
	ctx.ap->setTargetPitch(85*PI/180);
	ctx.controls->setElevator(-1);
	ctx.controls->setThrottle(1);
	do {
		yield();
	} while (ctx.fi->getCurrentPitch() < 80*PI/180);
	
	nfo = "Immelmann rotating.";
	ctx.ap->reset();
	float dest_course = 2*PI*RAND;
	float course;
	ctx.controls->setElevator(0);
	ctx.controls->setAileron(1);
	float error;
	do {
		yield();
		Vector up = ctx.actor->getUpVector();
		up[1]=0;
		up.normalize();
		course = atan2(up[2],up[0]);
		error = course-dest_course;
	    if (error < -PI) error += 2*PI;
	    if (error > PI) error -= 2*PI;
	} while (std::abs(error) > 5*PI/180);
	
	nfo = "Immelmann breaking out.";
	ctx.controls->setAileron(0);
	ctx.controls->setElevator(-1);
	ctx.controls->setThrottle(0);
	do{
		yield();
	} while (ctx.actor->getUpVector()[1] > -0.5);
	do{
		yield();
	} while (ctx.actor->getUpVector()[1] < 0.5);
	
}


void Dogfight::flyImmelmannTurn() {
	nfo = "Immelmann turn pulling up.";
	ctx.ap->reset();
	ctx.controls->setElevator(-1);
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] > sin(-70));
	
	nfo = "Immelmann turn rotating.";
	ctx.ap->setMode(AP_PITCH_MASK|AP_ROLL_MASK);
	ctx.ap->setTargetPitch(0);
	ctx.ap->setTargetRoll(0);
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] < sin(60));
}


void Dogfight::flySplitS() {
	nfo = "Split S rotating.";
	ctx.ap->setMode(AP_PITCH_MASK|AP_ROLL_MASK);
	ctx.ap->setTargetPitch(0);
	ctx.ap->setTargetRoll(PI); // upside down
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] > -0.8);
	
	nfo = "Split S pulling up.";
	ctx.ap->reset();
	ctx.controls->setElevator(-1);
	do {
		yield();
	} while (ctx.actor->getUpVector()[1] < 0.8);
}