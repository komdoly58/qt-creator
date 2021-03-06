/*
* Pthread Mutex
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_MUTEX_PTHREAD_H__
#define BOTAN_MUTEX_PTHREAD_H__

#include <botan/mutex.h>

namespace Botan {

/*
* Pthread Mutex Factory
*/
class BOTAN_DLL Pthread_Mutex_Factory : public Mutex_Factory
   {
   public:
      Mutex* make();
   };

}

#endif
