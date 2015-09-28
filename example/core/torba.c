/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl examples                                      */
/* Filename:      torba.c                                            */
/* Description:   Demonstrate TORBA usage                            */
/*                                                                   */
/*********************************************************************/

#include <malloc.h>
#include <av_torb.h>

/*!
 \file torba.c
 \example torba TORBA usage example
 \code
*/

/* av_foo.h */

/* defines TORBA class as a normal C structure */
typedef struct av_foo
{
	/* The first field must be TORBA class type derived by av_object */
	av_object_t object;

	/* Follows some specific to av_foo methods and fields */
	av_result_t (*do_foolish)(struct av_foo* self);
} av_foo_t, *av_foo_p;

/* export foo registrar function */
av_result_t av_foo_register_torba( void );

/* end of av_foo.h */

/* av_foo.c */

/* Member function definition to av_foo class */
static av_result_t av_foo_do_foolish(struct av_foo* self)
{
	AV_UNUSED(self);
	/* do really a big foolish here */
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
	self->do_foolish = av_foo_do_foolish;
	return AV_OK;
}

/* the only public function exported by av_foo.c */
av_result_t av_foo_register_torba( void )
{
	return av_torb_register_class("foo", "object", sizeof(av_foo_t), av_foo_constructor, av_foo_destructor);
}
/* end of av_foo.c */

/* foo_usage.c */

/* entry point */
int main(void)
{
	av_result_t rc;
	av_foo_p foo;

	/* Initialize TORBA */
	if (AV_OK != (rc = av_torb_init()))
		return rc;

	/* register av_foo class first */
	av_foo_register_torba();

	/* then create instance to av_foo class named `foo' */
	av_torb_create_object("foo", (av_object_p*)&foo);

	/* use the foo object */
	foo->do_foolish(foo);

	/* The expression below will be evaluated to AV_TRUE */
	av_assert(O_is_a(foo, "object") && O_is_a(foo, "foo"),
		"This will not happen as such defined av_foo class");

	/* destroy the foo object by calling its default destrucor */
	/* this is equivalent to av_torb_destroy_object(foo, "foo") */
	O_destroy(foo);

	/* Deinitialize TORBA */
	av_torb_done();
	return rc;
}

/* end of foo_usage.c */

/*! \endcode */
