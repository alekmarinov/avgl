/*********************************************************************/
/*                                                                   */
/* Copyright (C) 2007,  AVIQ Bulgaria Ltd                            */
/*                                                                   */
/* Project:       avgl                                               */
/* Filename:      av_scripting.h                                     */
/*                                                                   */
/*********************************************************************/

/*! \file av_scripting.h
*   \brief av_scripting definition
*/

#ifndef __AV_SCRIPTING_H
#define __AV_SCRIPTING_H

#include <av_oop.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* \brief scripting interface
*
*/
typedef struct av_scripting
{
	/*! Parent class object */
	av_object_t object;

	/*!
	* \brief Executes script file
	* \param self is a reference to this object
	* \param filename a script file to execute
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_EFOUND if script file not found
	*         - AV_ESCRIPT if error while executing script file
	*/
	av_result_t (*dofile)(struct av_scripting* self, const char* filename, int nargs, char* argv[]);

	/*!
	* \brief Executes bytecode given in a buffer
	* \param self is a reference to this object
	* \param buffer containing the bytecode
	* \param bufsize is the size of the buffer
	* \return av_result_t
	*         - AV_OK on success
	*         - AV_ESCRIPT if error while executing the script bytecode
	*/
	av_result_t (*dobuffer)(struct av_scripting* self,
							const char* buffer,
							int bufsize,
							const char* name);

} av_scripting_t, *av_scripting_p;

/*!
* \brief Registers scripting class into TORBA
* \return av_result_t
*         - AV_OK on success
*         - AV_EMEM on out of memory
*/
AV_API av_result_t av_scripting_register_torba(void);

#ifdef __cplusplus
}
#endif

#endif /* __AV_SCRIPTING_H */
