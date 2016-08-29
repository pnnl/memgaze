/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: miami_utils.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: A few useful functions.
 */

#include "miami_utils.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


namespace MIAMIU {
   using std::string;
   
   void ExtractNameAndPath(const string& input, string& path, string& name)
   {
       size_t pos = input.rfind('/');
       if (pos == std::string::npos)  // no path
       {
          path = ".";
          name = input;
       } else
       {
          path = input.substr(0, pos);
          name = input.substr(pos+1);
       }
   }
   
   std::string StripPath (const std::string&  path)
   {
       size_t pos = path.rfind('/');
       if (pos == std::string::npos)
          return (path);
       else
          return (path.substr(pos+1));
   }

   const char * StripPath (const char * path)
   {
       const char * file = strrchr (path,'/');
       if (file)
           return file+1;
       else
           return path;
   }

   int CopyFile(const char *to, const char *from)
   {
       int fd_to, fd_from;
       char buf[4096];
       ssize_t nread;
       int saved_errno;

       fd_from = open(from, O_RDONLY);
       if (fd_from < 0)
           return -1;

       fd_to = open(to, O_WRONLY | O_CREAT, 0666);
       if (fd_to < 0)
           goto out_error;

       while (nread = read(fd_from, buf, sizeof buf), nread > 0)
       {
           char *out_ptr = buf;
           ssize_t nwritten;

           do {
               nwritten = write(fd_to, out_ptr, nread);

               if (nwritten >= 0)
               {
                   nread -= nwritten;
                   out_ptr += nwritten;
               }
               else if (errno != EINTR)
               {
                   goto out_error;
               }
           } while (nread > 0);
       }

       if (nread == 0)
       {
           if (close(fd_to) < 0)
           {
               fd_to = -1;
               goto out_error;
           }
           close(fd_from);

           /* Success! */
           return 0;
       }

     out_error:
       saved_errno = errno;

       close(fd_from);
       if (fd_to >= 0)
           close(fd_to);

       errno = saved_errno;
       return -1;
   }

}
