/*
 * defaults.h
 *
 *  Created on: 17.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/config.h"
#include "ibrdtn/utils/Thread.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"
#include "ibrdtn/utils/Conditional.h"
#include "ibrdtn/utils/WaitForConditional.h"
#include "ibrdtn/utils/SyslogStream.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <limits>
#include <stdexcept>
#include <errno.h>
#include <unistd.h>

#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>

#if HAVE_STDINT_H
# include <stdint.h>
#endif

#include <time.h>
#include <math.h>

#include <vector>
#include <string>
#include <list>
#include <map>
#include <queue>

#include <csignal>
#include <sys/types.h>

using namespace std;

