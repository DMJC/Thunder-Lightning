#include <landscape.h>
#include <interfaces/IGame.h>
#include <SDL/SDL.h>
#include <modules/jogi/JOpenGLRenderer.h>
#include <remap.h>
#include <ActorStage.h>

class Status;
class ILoDQuadManager;
class ISkyBox;
class IMap;
namespace UI {
	class Console;
}

class Game: public IGame, public ActorStage, public SigObject
{
public:
    static Game * the_game;

    Game(int argc, const char **argv);
    ~Game();

    void run();

    virtual Ptr<TextureManager> getTexMan();
    virtual JRenderer *getRenderer();
    virtual EventRemapper *getEventRemapper();
    virtual Ptr<IModelMan> getModelMan();
    virtual Ptr<IConfig> getConfig();
    virtual Ptr<IActor> getCamPos();
    virtual void setCamPos(Ptr<IActor>);
    virtual Ptr<ICamera> getCamera();
    virtual Ptr<Clock> getClock();
    virtual Ptr<ITerrain> getTerrain();
    virtual Ptr<IPlayer> getPlayer();
    virtual Ptr<IGunsight> getGunsight();
    virtual Ptr<Environment> getEnvironment();
#if ENABLE_SKY
    virtual Ptr<ISky> getSky();
#endif
    virtual Ptr<IFontMan> getFontMan();
    virtual Ptr<SoundMan> getSoundMan();
    virtual Ptr<Collide::CollisionManager> getCollisionMan();
    virtual void getMouseState(float *mx, float *my, int *buttons);
    virtual double  getTimeDelta();
    virtual double  getTime();
    virtual void drawDebugTriangleAt(const Vector & p);

    virtual void clearScreen();

private:
    void initModules(Status &);

    void initControls();

    void doEvents();

    void preFrame();
    void doFrame();
    void postFrame();

    void setupRenderer();

    void drawDebugTriangle();

    void endGame();

public:
    void togglePauseMode();
private:
    void toggleFollowMode();

    void accelerateSpeed();
    void decelerateSpeed();

private:
    int argc;
    const char **argv;

    SDL_Surface *surface;
    JOpenGLRenderer *renderer;

    Ptr<TextureManager> texman;

    EventRemapper * event_remapper;
    KeyboardSignal keyboard_sig;

    Ptr<IPlayer> player;
    Ptr<IActor> cam_pos;
    Ptr<ICamera> camera;
    Ptr<Clock> clock;
    Ptr<IConfig> config;
    Ptr<IFontMan> fontman;
    Ptr<SoundMan> soundman;
    Ptr<IModelMan> modelman;
    Ptr<Collide::CollisionManager> collisionman;
#if ENABLE_LOD_TERRAIN
    Ptr<ILoDQuadManager> quadman;
#endif
#if ENABLE_SKYBOX
    Ptr<ISkyBox> skybox;
#endif
#if ENABLE_MAP
    Ptr<IMap> map;
#endif
#if ENABLE_GUNSIGHT
    Ptr<IGunsight> gunsight;
#endif
    Ptr<Environment> environment;

    Ptr<UI::Console> console;

    Uint32 ticks_now, ticks_old;
    int mouse_relx, mouse_rely;
    int mouse_buttons;
    jBool game_done;

    bool key_tab_old;
    bool pause;
    bool follow_mode;
    float game_speed;
    int view;
};