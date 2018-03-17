 /**
  * Copyright 2018 Comcast Cable Communications Management, LLC
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void dbg_err (int err, const char *fmt, ...)
{   
    char errbuf[101] = {0};
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);

    if( 0 == strerror_r (err, errbuf, 100) )
    {   
        printf("%s\n", errbuf);
    }
    else
    {   
        printf("strerror_r returned failure!\n");
    } 
}

