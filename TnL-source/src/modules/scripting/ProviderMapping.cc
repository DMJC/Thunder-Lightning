#ifdef HAVE_IO
// ---------------------------------------------------------------
// |  ProviderMapping                                            |
// ---------------------------------------------------------------

#include <interfaces/IPositionProvider.h>
#include <interfaces/IMovementProvider.h>
#include <modules/math/SpecialMatrices.h>
#include "mappings.h"

namespace {
	
	struct PositionProviderMapping
        :	public TemplatedObjectMapping<IPositionProvider, DynamicCastMapping<IPositionProvider> >
	{
        static const char *const id;
        
		static void addMapping(Ptr<IGame> thegame, IoState * state) {
			IoObject * self =  proto(state);
			IoState_registerProtoWithId_(state, self, id);
			IoObject_setSlot_to_(state->lobby, IOSYMBOL("PositionProvider"), self);
		}
		static IoObject *proto(void *state) {
			IoMethodTable methodTable[] = {
				/* standard I/O */
				{"getLocation", getLocation},
				{"getUpVector", getUpVector},
				{"getRightVector", getRightVector},
				{"getFrontVector", getFrontVector},
				{"getOrientation", getOrientation},
				{NULL, NULL}
			};
			IoObject *self = IoObject_new(state);
			IoObject_tag_(self, tag(state, "PositionProvider"));
			IoObject_setDataPointer_(self, 0);
			
			IoObject_addTaglessMethodTable_(self, methodTable);
			return self;
		}
		
		static IoObject * getOrientation
		(IoObject *self, IoObject *locals, IoMessage *m) {
			BEGIN_FUNC("PositionProvider.getOrientation")
			Vector up,right,front;
			getObject(self)->getOrientation(&up,&right,&front);
			return wrapObject<Matrix3>(MatrixFromColumns(right,up,front), IOSTATE);
		}
		CREATE_FUNC(IPositionProvider, id)
		
		GET_VECTOR(getLocation)
		GET_VECTOR(getUpVector)
		GET_VECTOR(getFrontVector)
		GET_VECTOR(getRightVector)
	};
    
    const char *const PositionProviderMapping::id = "PositionProvider";
	
	struct MovementProviderMapping
	:	public TemplatedObjectMapping<IMovementProvider, DynamicCastMapping<IMovementProvider> >
	{
        static const char *const id;
        
		static void addMapping(Ptr<IGame> thegame, IoState * state) {
			IoObject * self =  proto(state);
			IoState_registerProtoWithId_(state, self, id);
			IoObject_setSlot_to_(state->lobby, IOSYMBOL("MovementProvider"), self);
			IoObject_setDataPointer_(self, 0);
		}
		static IoObject *proto(void *state) {
			IoMethodTable methodTable[] = {
				{"getMovementVector", getMovementVector},
				{NULL, NULL}
			};
			IoObject *self = IoObject_new(state);
			IoObject_tag_(self, tag(state, "MovementProvider"));
			IoObject_setDataPointer_(self, 0);
			
			IoObject_addTaglessMethodTable_(self, methodTable);
			return self;
		}
		CREATE_FUNC(IMovementProvider, id)
		
		GET_VECTOR(getMovementVector)
	};
    
    const char *const MovementProviderMapping::id = "MovementProvider";
}

template<>
void addMapping<IPositionProvider>(Ptr<IGame> game, IoState *state) {
	PositionProviderMapping::addMapping(game,state);
}

template<>
IoObject * wrapObject<Ptr<IPositionProvider> >
(Ptr<IPositionProvider> pp, IoState *state) {
	return PositionProviderMapping::create(pp, state);
}

template<>
Ptr<IPositionProvider> unwrapObject<Ptr<IPositionProvider> >(IoObject * self) {
    return PositionProviderMapping::getObject(self);
}


template<>
IoObject *getProtoObject<Ptr<IPositionProvider> >(IoState * state) {
	return IoState_protoWithId_(state, PositionProviderMapping::id);
}


template<>
void addMapping<IMovementProvider>(Ptr<IGame> game, IoState *state) {
	MovementProviderMapping::addMapping(game,state);
}


template<>
IoObject * wrapObject
(Ptr<IMovementProvider> pp, IoState *state) {
	return MovementProviderMapping::create(pp, state);
}

template<>
Ptr<IMovementProvider> unwrapObject(IoObject * self) {
	return MovementProviderMapping::getObject(self);
}

template<>
IoObject *getProtoObject<Ptr<IMovementProvider> >(IoState * state) {
	return IoState_protoWithId_(state, MovementProviderMapping::id);
}

#endif // HAVE_IO
