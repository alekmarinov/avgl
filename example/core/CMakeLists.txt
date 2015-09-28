# example/core/CMakeLists.txt

ADD_EXECUTABLE(torba torba.c)
ADD_EXECUTABLE(logging logging.c)
ADD_EXECUTABLE(prefs prefs.c)
ADD_EXECUTABLE(mutex mutex.c)
ADD_EXECUTABLE(list list.c)
ADD_EXECUTABLE(hash hash.c)

IF(WITH_PTHREADS)
	ADD_EXECUTABLE(threads threads.c)
	ADD_EXECUTABLE(syncqueue syncqueue.c)
ENDIF(WITH_PTHREADS)
