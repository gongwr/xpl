/* XPL - Library of useful routines for C programming
 * Copyright (C) 2000  Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* A test program for the main loop and IO channel code.
 * Just run it. Optional parameter is number of sub-processes.
 */

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include "config.h"

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef G_OS_WIN32
  #include <io.h>
  #include <fcntl.h>
  #include <process.h>
  #define STRICT
  #include <windows.h>
  #define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif

#ifdef G_OS_UNIX
  #include <unistd.h>
#endif

static int nrunning;
static xmain_loop_t *main_loop;

#define BUFSIZE 5000		/* Larger than the circular buffer in
				 * giowin32.c on purpose.
				 */

static int nkiddies;

static struct {
  int fd;
  int seq;
} *seqtab;

static GIOError
read_all (int         fd,
	  xio_channel_t *channel,
	  char       *buffer,
	  xuint_t       nbytes,
	  xuint_t      *bytes_read)
{
  xuint_t left = nbytes;
  xsize_t nb;
  GIOError error = G_IO_ERROR_NONE;
  char *bufp = buffer;

  /* g_io_channel_read() doesn't necessarily return all the
   * data we want at once.
   */
  *bytes_read = 0;
  while (left)
    {
      error = g_io_channel_read (channel, bufp, left, &nb);

      if (error != G_IO_ERROR_NONE)
	{
	  g_print ("gio-test: ...from %d: %d\n", fd, error);
	  if (error == G_IO_ERROR_AGAIN)
	    continue;
	  break;
	}
      if (nb == 0)
	return error;
      left -= nb;
      bufp += nb;
      *bytes_read += nb;
    }
  return error;
}

static void
shutdown_source (xpointer_t data)
{
  if (xsource_remove (*(xuint_t *) data))
    {
      nrunning--;
      if (nrunning == 0)
	xmain_loop_quit (main_loop);
    }
}

static xboolean_t
recv_message (xio_channel_t  *channel,
	      xio_condition_t cond,
	      xpointer_t    data)
{
  xint_t fd = g_io_channel_unix_get_fd (channel);
  xboolean_t retval = TRUE;

  g_debug ("gio-test: ...from %d:%s%s%s%s", fd,
	   (cond & G_IO_ERR) ? " ERR" : "",
	   (cond & G_IO_HUP) ? " HUP" : "",
	   (cond & G_IO_IN)  ? " IN"  : "",
	   (cond & G_IO_PRI) ? " PRI" : "");

  if (cond & (G_IO_ERR | G_IO_HUP))
    {
      shutdown_source (data);
      retval = FALSE;
    }

  if (cond & G_IO_IN)
    {
      char buf[BUFSIZE];
      xuint_t nbytes = 0;
      xuint_t nb;
      xuint_t j;
      int i, seq;
      GIOError error;

      error = read_all (fd, channel, (xchar_t *) &seq, sizeof (seq), &nb);
      if (error == G_IO_ERROR_NONE)
	{
	  if (nb == 0)
	    {
	      g_debug ("gio-test: ...from %d: EOF", fd);
	      shutdown_source (data);
	      return FALSE;
	    }

	  g_assert (nb == sizeof (nbytes));

	  for (i = 0; i < nkiddies; i++)
	    if (seqtab[i].fd == fd)
	      {
                g_assert_cmpint (seq, ==, seqtab[i].seq);
		seqtab[i].seq++;
		break;
	      }

	  error = read_all (fd, channel, (xchar_t *) &nbytes, sizeof (nbytes), &nb);
	}

      if (error != G_IO_ERROR_NONE)
	return FALSE;

      if (nb == 0)
	{
	  g_debug ("gio-test: ...from %d: EOF", fd);
	  shutdown_source (data);
	  return FALSE;
	}

      g_assert (nb == sizeof (nbytes));

      g_assert_cmpint (nbytes, <, BUFSIZE);
      g_assert (nbytes < BUFSIZE);
      g_debug ("gio-test: ...from %d: %d bytes", fd, nbytes);
      if (nbytes > 0)
	{
	  error = read_all (fd, channel, buf, nbytes, &nb);

	  if (error != G_IO_ERROR_NONE)
	    return FALSE;

	  if (nb == 0)
	    {
	      g_debug ("gio-test: ...from %d: EOF", fd);
	      shutdown_source (data);
	      return FALSE;
	    }

	  for (j = 0; j < nbytes; j++)
            g_assert (buf[j] == ' ' + (char) ((nbytes + j) % 95));
	  g_debug ("gio-test: ...from %d: OK", fd);
	}
    }
  return retval;
}

#ifdef G_OS_WIN32

static xboolean_t
recv_windows_message (xio_channel_t  *channel,
		      xio_condition_t cond,
		      xpointer_t    data)
{
  GIOError error;
  MSG msg;
  xsize_t nb;

  while (1)
    {
      error = g_io_channel_read (channel, (xchar_t *) &msg, sizeof (MSG), &nb);

      if (error != G_IO_ERROR_NONE)
	{
	  g_print ("gio-test: ...reading Windows message: G_IO_ERROR_%s\n",
		   (error == G_IO_ERROR_AGAIN ? "AGAIN" :
		    (error == G_IO_ERROR_INVAL ? "INVAL" :
		     (error == G_IO_ERROR_UNKNOWN ? "UNKNOWN" : "???"))));
	  if (error == G_IO_ERROR_AGAIN)
	    continue;
	}
      break;
    }

  g_print ("gio-test: ...Windows message for 0x%p: %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT "\n",
	   msg.hwnd, msg.message, msg.wParam, (gintptr)msg.lParam);

  return TRUE;
}

LRESULT CALLBACK window_procedure (HWND   hwnd,
                                   UINT   message,
                                   WPARAM wparam,
                                   LPARAM lparam);

LRESULT CALLBACK
window_procedure (HWND hwnd,
		  UINT message,
		  WPARAM wparam,
		  LPARAM lparam)
{
  g_print ("gio-test: window_procedure for 0x%p: %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT "\n",
	   hwnd, message, wparam, (gintptr)lparam);
  return DefWindowProc (hwnd, message, wparam, lparam);
}

#endif

int
main (int    argc,
      char **argv)
{
  if (argc < 3)
    {
      /* Parent */

      xio_channel_t *my_read_channel;
      xchar_t *cmdline;
      int i;
#ifdef G_OS_WIN32
      GTimeVal start, end;
      xpollfd_t pollfd;
      int pollresult;
      ATOM klass;
      static WNDCLASS wcl;
      HWND hwnd;
      xio_channel_t *windows_messages_channel;
#endif

      nkiddies = (argc == 1 ? 1 : atoi(argv[1]));
      seqtab = g_malloc (nkiddies * 2 * sizeof (int));

#ifdef G_OS_WIN32
      wcl.style = 0;
      wcl.lpfnWndProc = window_procedure;
      wcl.cbClsExtra = 0;
      wcl.cbWndExtra = 0;
      wcl.hInstance = GetModuleHandle (NULL);
      wcl.hIcon = NULL;
      wcl.hCursor = NULL;
      wcl.hbrBackground = NULL;
      wcl.lpszMenuName = NULL;
      wcl.lpszClassName = "gio-test";

      klass = RegisterClass (&wcl);

      if (!klass)
	{
	  g_print ("gio-test: RegisterClass failed\n");
	  exit (1);
	}

      hwnd = CreateWindow (MAKEINTATOM(klass), "gio-test", 0, 0, 0, 10, 10,
			   NULL, NULL, wcl.hInstance, NULL);
      if (!hwnd)
	{
	  g_print ("gio-test: CreateWindow failed\n");
	  exit (1);
	}

      windows_messages_channel = g_io_channel_win32_new_messages ((xuint_t) (guintptr) hwnd);
      g_io_add_watch (windows_messages_channel, G_IO_IN, recv_windows_message, 0);
#endif

      for (i = 0; i < nkiddies; i++)
	{
	  int pipe_to_sub[2], pipe_from_sub[2];
	  xuint_t *id;

	  if (pipe (pipe_to_sub) == -1 ||
	      pipe (pipe_from_sub) == -1)
	    perror ("pipe"), exit (1);

	  seqtab[i].fd = pipe_from_sub[0];
	  seqtab[i].seq = 0;

	  my_read_channel = g_io_channel_unix_new (pipe_from_sub[0]);

	  id = g_new (xuint_t, 1);
	  *id =
	    g_io_add_watch_full (my_read_channel,
				 G_PRIORITY_DEFAULT,
				 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
				 recv_message,
				 id, g_free);

	  nrunning++;

#ifdef G_OS_WIN32
	  cmdline = xstrdup_printf ("%d:%d:0x%p",
				     pipe_to_sub[0],
				     pipe_from_sub[1],
				     hwnd);
	  _spawnl (_P_NOWAIT, argv[0], argv[0], "--child", cmdline, NULL);
#else
	  cmdline = xstrdup_printf ("%s --child %d:%d &", argv[0],
				     pipe_to_sub[0], pipe_from_sub[1]);

	  system (cmdline);
          g_free (cmdline);
#endif
	  close (pipe_to_sub[0]);
	  close (pipe_from_sub [1]);

#ifdef G_OS_WIN32
	  g_get_current_time (&start);
	  g_io_channel_win32_make_pollfd (my_read_channel, G_IO_IN, &pollfd);
	  pollresult = g_io_channel_win32_poll (&pollfd, 1, 100);
	  g_get_current_time (&end);
	  if (end.tv_usec < start.tv_usec)
	    end.tv_sec--, end.tv_usec += 1000000;
	  g_print ("gio-test: had to wait %ld.%03ld s, result:%d\n",
		   end.tv_sec - start.tv_sec,
		   (end.tv_usec - start.tv_usec) / 1000,
		   pollresult);
#endif
          g_io_channel_unref (my_read_channel);
	}

      main_loop = xmain_loop_new (NULL, FALSE);

      xmain_loop_run (main_loop);

      xmain_loop_unref (main_loop);
      g_free (seqtab);
    }
  else if (argc == 3)
    {
      /* Child */

      int readfd, writefd;
#ifdef G_OS_WIN32
      HWND hwnd;
#endif
      int i, j;
      char buf[BUFSIZE];
      int buflen;
      GTimeVal tv;
      int n;

      g_get_current_time (&tv);

      sscanf (argv[2], "%d:%d%n", &readfd, &writefd, &n);

#ifdef G_OS_WIN32
      sscanf (argv[2] + n, ":0x%p", &hwnd);
#endif

      srand (tv.tv_sec ^ (tv.tv_usec / 1000) ^ readfd ^ (writefd << 4));

      for (i = 0; i < 20 + rand() % 20; i++)
	{
	  g_usleep (100 + (rand() % 10) * 5000);
	  buflen = rand() % BUFSIZE;
	  for (j = 0; j < buflen; j++)
	    buf[j] = ' ' + ((buflen + j) % 95);
	  g_debug ("gio-test: child writing %d+%d bytes to %d",
		   (int)(sizeof(i) + sizeof(buflen)), buflen, writefd);
	  write (writefd, &i, sizeof (i));
	  write (writefd, &buflen, sizeof (buflen));
	  write (writefd, buf, buflen);

#ifdef G_OS_WIN32
	  if (rand() % 100 < 5)
	    {
	      int msg = WM_USER + (rand() % 100);
	      WPARAM wparam = rand ();
	      LPARAM lparam = rand ();
	      g_print ("gio-test: child posting message %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT " to 0x%p\n",
		       msg, wparam, (gintptr)lparam, hwnd);
	      PostMessage (hwnd, msg, wparam, lparam);
	    }
#endif
	}
      g_debug ("gio-test: child exiting, closing %d", writefd);
      close (writefd);
    }
  else
    g_print ("Huh?\n");

  return 0;
}
