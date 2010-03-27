#ifndef _VIRTUAL_H
#define _VIRTUAL_H

#define VIRTUAL_FUNC_VOID(call_name, args...)({\
	if( self->ops && self->ops->call_name )\
		self->ops->call_name(self, args);})


#define VIRTUAL_FUNC(call_name, args...)({\
	if( self->ops && self->ops->call_name )\
		return self->ops->call_name(self, args);\
	return 0;})

#endif // _VIRTUAL_H
