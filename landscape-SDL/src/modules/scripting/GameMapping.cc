// ---------------------------------------------------------------
// |  GameMapping                                                |
// ---------------------------------------------------------------

#include <interfaces/IActor.h>
#include <interfaces/IView.h>
#include "mappings.h"


template<>
Ptr<IGame> unwrapObject<Ptr<IGame> >(IoObject * self) {
	return (IGame*)self->data;
}



namespace {
	
	struct GameMapping : public TemplatedObjectMapping<IGame> {
		static void addMapping(Ptr<IGame> thegame, IoState * state) {
			IoObject *object = state->mainActor;
			IoObject *lobby = state->lobby;
			
			IoObject *self = proto(state);
			IoState_registerProtoWithFunc_(state, self, proto);
			IoObject_setSlot_to_(lobby,IOSTRING("Game"), self);
			retarget(self, ptr(thegame));
		}
		
		static IoObject *proto(void *state) {
			IoMethodTable methodTable[] = {
				{"asActorStage", castfunc<Ptr<IGame>, Ptr<IActorStage> >},
				{"getControlledActor", getControlledActor},
				{"setControlledActor", setControlledActor},
				{"setView", setView},
				{"getViewSubject", getViewSubject},
				{NULL, NULL}
			};
			IoObject *self = IoObject_new(state);
			self->tag = tag(state, "Game");
			self->data = 0;
			IoObject_addMethodTable_(self, methodTable);
			return self;
		}
	
		static IoObject *
		getControlledActor(IoObject *self, IoObject *locals, IoMessage *m) {
			BEGIN_FUNC("Game.getControlledActor")
			return wrapObject<Ptr<IActor> >(
				getObject(self)->getCurrentlyControlledActor(),
				IOSTATE);
		}
		
		static IoObject *
		setControlledActor(IoObject *self, IoObject *locals, IoMessage *m) {
			BEGIN_FUNC("Game.setControlledActor")
			IOASSERT(IoMessage_argCount(m) == 1,"Expected one argument")
			IoObject *arg = IoMessage_locals_objectArgAt_(m, locals, 0);
			Ptr<IActor> a = unwrapObject<Ptr<IActor> >(arg);
			getObject(self)->setCurrentlyControlledActor( a );
			return self;
		}
		
		static IoObject *
		setView(IoObject *self, IoObject *locals, IoMessage *m) {
			BEGIN_FUNC("Game.setView")
			IOASSERT(IoMessage_argCount(m) == 2,"Expected two arguments")
			Ptr<IActor> a = unwrapObject<Ptr<IActor> >(
				IoMessage_locals_objectArgAt_(m, locals, 0));
		    int view = unwrapObject<int>(
				IoMessage_locals_objectArgAt_(m, locals, 1));
			getObject(self)->setCurrentView(a->getView(view));
			return self;
		}
		
		static IoObject *
		getViewSubject(IoObject *self, IoObject *locals, IoMessage *m) {
			BEGIN_FUNC("Game.getViewSubject")
			Ptr<IView> view = getObject(self)->getCurrentView();
			if (!view) return IONIL(self);
			Ptr<IActor> subject = view->getViewSubject();
			if (!subject) return IONIL(self);
			return wrapObject<Ptr<IActor> >(subject, IOSTATE);
		}
		
	};
}

template<>
void addMapping<IGame>(Ptr<IGame> game, IoState *state) {
	GameMapping::addMapping(game,state);
}

template<>
IoObject *getProtoObject<IGame>(IoState * state) {
	return IoState_protoWithInitFunction_(state, GameMapping::proto);
}
