
#include <avgl.h>

/* defines a class as a normal C structure */
typedef struct _av_foo_t
{
	/* The first field must be TORBA class type derived by av_object */
	av_object_t object;

	int invoked;

	/* foo specific method */
	av_result_t(*do_foolish)(struct _av_foo_t* self);
} av_foo_t, *av_foo_p;

/* Member function definition to av_foo class */
static av_result_t av_foo_do_foolish(struct _av_foo_t* self)
{
	AV_UNUSED(self);
	/* do really a big foolish here */
	self->invoked = 1;
	return AV_OK;
}

/* av_foo destructor must have exactly this prototype */
static void av_foo_destructor(void* pobject)
{
	AV_UNUSED(pobject);
	/* destroy related to self allocations */
}

/* the av_foo constructor body */
static av_result_t av_foo_constructor(av_object_p object)
{
	av_foo_p self = (av_foo_p)object;
	/* initialize av_foo class */
	self->invoked = 0;
	self->do_foolish = av_foo_do_foolish;
	return AV_OK;
}

/* inherite a class */
typedef struct _av_boo_t
{
	/* The first field must be the inherited class */
	av_foo_t foo;

	int invoked;

	/* boo specific method */
	av_result_t(*do_boolish)(struct _av_boo_t* self);
} av_boo_t, *av_boo_p;

/* Member function definition to av_boo class */
static av_result_t av_foo_do_boolish(struct _av_boo_t* self)
{
	AV_UNUSED(self);
	/* do really a big boolish here */
	self->invoked = 1;
	return AV_OK;
}

/* av_boo destructor must have exactly this prototype */
static void av_boo_destructor(void* pobject)
{
	AV_UNUSED(pobject);
	/* destroy related to self allocations */
}

/* the av_boo constructor body */
static av_result_t av_boo_constructor(av_object_p object)
{
	av_boo_p self = (av_boo_p)object;
	/* initialize av_boo class */
	self->invoked = 0;
	self->do_boolish = av_foo_do_boolish;
	return AV_OK;
}

int test_oop_inheritance()
{
	int status;
	av_oop_p oop;
	av_boo_p boo;
	av_oop_create(&oop);
	oop->define_class(oop, "foo", NULL, sizeof(av_foo_t), av_foo_constructor, av_foo_destructor);
	oop->define_class(oop, "boo", "foo", sizeof(av_boo_t), av_boo_constructor, av_boo_destructor);
	oop->new(oop, "boo", (av_object_p*)&boo);
	boo->do_boolish(boo);
	status = boo->invoked == 1 && boo->foo.invoked == 0;
	O_destroy(boo);
	oop->destroy(oop);
	return status;
}

