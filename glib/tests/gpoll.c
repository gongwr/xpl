/* Unit test for W32 version of g_poll()
 *
 * Copyright © 2017 Руслан Ижбулатов <lrn1986@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <winsock2.h>

#define NUM_POLLEES 999
#define NUM_POLLFDS 1000

#define ASYNC_CONNECT_OK(r) (r == 0 || (r < 0 && GetLastError () == WSAEWOULDBLOCK))

#define REPEAT 1

static void
init_networking (void)
{
  WSADATA wsadata;

  if (WSAStartup (MAKEWORD (2, 0), &wsadata) != 0)
    xerror ("Windows Sockets could not be initialized");
}

static void
prepare_fds (SOCKET  sockets[],
             xpollfd_t fds[],
             int     num_pollees)
{
  xint_t i;

  for (i = 0; i < num_pollees; i++)
    {
      fds[i].fd = (gintptr) WSACreateEvent ();
      xassert (WSAEventSelect (sockets[i], (HANDLE) fds[i].fd, FD_READ | FD_CLOSE) == 0);
    }
}

static void
reset_fds (xpollfd_t fds[],
           int     num_pollees)
{
  xint_t i;

  for (i = 0; i < num_pollees; i++)
    {
      WSAResetEvent ((HANDLE) fds[i].fd);
      fds[i].events =  G_IO_IN | G_IO_OUT | G_IO_ERR;
      fds[i].revents = 0;
    }
}

static void
reset_fds_msg (xpollfd_t fds[],
               int     num_pollfds)
{
  fds[num_pollfds - 1].fd = G_WIN32_MSG_HANDLE;
  fds[num_pollfds - 1].events = G_IO_IN;
  fds[num_pollfds - 1].revents = 0;
}

static void
check_fds (SOCKET  sockets[],
           xpollfd_t fds[],
           int     num_pollees)
{
  xint_t i;

  for (i = 0; i < num_pollees; i++)
    {
      if (fds[i].revents != 0)
        {
          WSANETWORKEVENTS events;
          xassert (WSAEnumNetworkEvents (sockets[i], 0, &events) == 0);

          fds[i].revents = 0;
          if (events.lNetworkEvents & (FD_READ | FD_ACCEPT))
            fds[i].revents |= G_IO_IN;

          if (events.lNetworkEvents & FD_WRITE)
            fds[i].revents |= G_IO_OUT;
          else
            {
              /* We have called WSAEnumNetworkEvents() above but it didn't
               * set FD_WRITE.
               */
              if (events.lNetworkEvents & FD_CONNECT)
                {
                  if (events.iErrorCode[FD_CONNECT_BIT] == 0)
                    fds[i].revents |= G_IO_OUT;
                  else
                    fds[i].revents |= (G_IO_HUP | G_IO_ERR);
                }
              if (fds[i].revents == 0 && (events.lNetworkEvents & (FD_CLOSE)))
                fds[i].revents |= G_IO_HUP;
            }
        }
    }
}

static void
prepare_sockets (SOCKET  sockets[],
                 SOCKET  opp_sockets[],
                 xpollfd_t fds[],
                 int     num_pollees)
{
  xint_t i;
  SOCKET server;
  struct sockaddr_in sa;
  unsigned long ul = 1;
  int sa_size;
  int r;

  server = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  xassert (server != INVALID_SOCKET);

  memset(&sa, 0, sizeof sa);

  sa.sin_family = AF_INET;
  sa.sin_port = 0;
  sa.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  sa_size = sizeof (sa);

  xassert (bind (server, (const struct sockaddr *) &sa, sa_size) == 0);
  xassert (getsockname (server, (struct sockaddr *) &sa, &sa_size) == 0);
  xassert (listen (server, 1) == 0);

  for (i = 0; i < num_pollees; i++)
    {
      opp_sockets[i] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
      xassert (opp_sockets[i] != INVALID_SOCKET);
      xassert (ioctlsocket (opp_sockets[i], FIONBIO, &ul) == 0);

      r = connect (opp_sockets[i], (const struct sockaddr *) &sa, sizeof (sa));
      xassert (ASYNC_CONNECT_OK (r));

      sockets[i] = accept (server, NULL, NULL);
      xassert (sockets[i] != INVALID_SOCKET);
      xassert (ioctlsocket (sockets[i], FIONBIO, &ul) == 0);
    }

  closesocket (server);
}

static void
cleanup_sockets (SOCKET sockets[],
                 SOCKET opp_sockets[],
                 int    num_pollees)
{
  xint_t i;

  for (i = 0; i < num_pollees; i++)
    {
      closesocket (sockets[i]);
      closesocket (opp_sockets[i]);
    }
}

static void
bucketize (sint64_t val,
           xint_t   buckets[],
           sint64_t bucket_limits[],
           xint_t   count)
{
  xint_t i;

  if (val > bucket_limits[count - 1])
    {
      buckets[count - 1] += 1;
      return;
    }

  for (i = count - 1; i > 0; i--)
    if (val < bucket_limits[i] && val >= bucket_limits[i - 1])
      {
        buckets[i] += 1;
        return;
      }

  buckets[0] += 1;
}

static void
print_buckets (xint_t   buckets[],
               sint64_t bucket_limits[],
               xint_t   count)
{
  xint_t i;

  for (i = 0; i < count; i++)
    if (i < count - 1)
      g_print ("%-4lld-%4lld|", i == 0 ? 0 : bucket_limits[i - 1], bucket_limits[i] - 1);
    else
      g_print ("  >= %-4lld|", bucket_limits[i - 1]);

  g_print ("\n");

  for (i = 0; i < count; i++)
    {
      xint_t len;
      xint_t padding;
      xint_t j;
      if (buckets[i] < 10)
        len = 1;
      else if (buckets[i] < 100)
        len = 2;
      else if (buckets[i] < 1000)
        len = 3;
      else
        len = 4;
      padding = 9 - len;
      for (j = 0; j < padding / 2; j++)
        g_print (" ");
      if (buckets[i] != 0)
        g_print ("%*d", len, buckets[i]);
      else
        g_print (" ");
      for (j = padding / 2; j < padding; j++)
        g_print (" ");
      g_print (" ");
    }

  g_print ("\n\n");
}

static void
test_gpoll (void)
{
  SOCKET sockets[NUM_POLLEES];
  xpollfd_t fds[NUM_POLLFDS];
  SOCKET opp_sockets[NUM_POLLEES];
  xint_t i;
  xint_t activatable;
  sint64_t times[REPEAT][2];
#define BUCKET_COUNT 25
  sint64_t bucket_limits[BUCKET_COUNT] = {3, 5, 10, 15, 20, 25, 30, 35, 40, 50, 60, 70, 80, 90, 100, 120, 150, 180, 220, 280, 350, 450, 600, 800, 1000};
  xint_t   buckets[BUCKET_COUNT];
  sint64_t times_avg = 0, times_min = G_MAXINT64, times_max = 0;

  prepare_sockets (sockets, opp_sockets, fds, NUM_POLLEES);
  prepare_fds (sockets, fds, NUM_POLLEES);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r;
      sint64_t diff;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 0);
      times[i][1] = g_get_monotonic_time ();
      xassert (r == 0);
      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("\nempty poll time:\n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  activatable = 0;

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;
      MSG msg;
      xboolean_t found_app;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = send (opp_sockets[activatable], (const char *) &t, 1, 0);
      xassert (PostMessage (NULL, WM_APP, 1, 2));
      /* This is to ensure that all sockets catch up, otherwise some might not poll active */
      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();

      check_fds (sockets, fds, NUM_POLLEES);
      v = recv (sockets[activatable], (char *) &t, 1, 0);
      found_app = FALSE;
      while (!found_app && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        if (msg.message == WM_APP && msg.wParam == 1 && msg.lParam == 2)
          found_app = TRUE;
      xassert (s == 1);
      xassert (r == 2);
      xassert (v == 1);
      xassert (found_app);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);
      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      activatable = (activatable + 1) % NUM_POLLEES;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("1-socket + msg poll time:\n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  activatable = 0;

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = send (opp_sockets[activatable], (const char *) &t, 1, 0);

      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();

      check_fds (sockets, fds, NUM_POLLEES);
      v = recv (sockets[activatable], (char *) &t, 1, 0);
      xassert (s == 1);
      xassert (r == 1);
      xassert (v == 1);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);

      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      activatable = (activatable + 1) % NUM_POLLEES;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("1-socket poll time:\n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;
      xint_t j;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = v = 0;

      for (j = 0; j < NUM_POLLEES / 2; j++)
        s += send (opp_sockets[j], (const char *) &t, 1, 0) == 1 ? 1 : 0;

      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();
      check_fds (sockets, fds, NUM_POLLEES);
      for (j = 0; j < NUM_POLLEES / 2; j++)
        v += recv (sockets[j], (char *) &t, 1, 0) == 1 ? 1 : 0;
      xassert (s == NUM_POLLEES / 2);
      xassert (r == NUM_POLLEES / 2);
      xassert (v == NUM_POLLEES / 2);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);

      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("half-socket poll time:\n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;
      xint_t j;
      MSG msg;
      xboolean_t found_app;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = v = 0;

      for (j = 0; j < NUM_POLLEES / 2; j++)
        s += send (opp_sockets[j], (const char *) &t, 1, 0) == 1 ? 1 : 0;
      xassert (PostMessage (NULL, WM_APP, 1, 2));

      /* This is to ensure that all sockets catch up, otherwise some might not poll active */
      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();
      check_fds (sockets, fds, NUM_POLLEES);
      for (j = 0; j < NUM_POLLEES / 2; j++)
        v += recv (sockets[j], (char *) &t, 1, 0) == 1 ? 1 : 0;
      found_app = FALSE;
      while (!found_app && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        if (msg.message == WM_APP && msg.wParam == 1 && msg.lParam == 2)
          found_app = TRUE;
      xassert (s == NUM_POLLEES / 2);
      xassert (r == NUM_POLLEES / 2 + 1);
      xassert (v == NUM_POLLEES / 2);
      xassert (found_app);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);

      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("half-socket + msg poll time:\n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;
      xint_t j;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = v = 0;

      for (j = 0; j < NUM_POLLEES; j++)
        s += send (opp_sockets[j], (const char *) &t, 1, 0) == 1 ? 1 : 0;

      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();
      check_fds (sockets, fds, NUM_POLLEES);
      for (j = 0; j < NUM_POLLEES; j++)
        v += recv (sockets[j], (char *) &t, 1, 0) == 1 ? 1 : 0;
      xassert (s == NUM_POLLEES);
      xassert (r == NUM_POLLEES);
      xassert (v == NUM_POLLEES);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);

      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
    }

  times_avg /= NUM_POLLEES;
  g_print ("%d-socket poll time: \n%4lldns - %4lldns, average %4lldns\n", NUM_POLLEES, times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  activatable = 0;
  times_avg = 0;
  times_min = G_MAXINT64;
  times_max = 0;
  memset (buckets, 0, sizeof (xint_t) * BUCKET_COUNT);

  for (i = 0; i < REPEAT; i++)
    {
      xint_t r, s, v, t;
      sint64_t diff;
      xint_t j;
      MSG msg;
      xboolean_t found_app;

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      s = v = 0;

      for (j = 0; j < activatable; j++)
        s += send (opp_sockets[j], (const char *) &t, 1, 0) == 1 ? 1 : 0;
      xassert (PostMessage (NULL, WM_APP, 1, 2));

      g_usleep (G_USEC_PER_SEC / 1000);

      times[i][0] = g_get_monotonic_time ();
      r = g_poll (fds, NUM_POLLFDS, 1000);
      times[i][1] = g_get_monotonic_time ();
      check_fds (sockets, fds, NUM_POLLEES);
      for (j = 0; j < activatable; j++)
        v += recv (sockets[j], (char *) &t, 1, 0) == 1 ? 1 : 0;
      found_app = FALSE;
      while (!found_app && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        if (msg.message == WM_APP && msg.wParam == 1 && msg.lParam == 2)
          found_app = TRUE;
      xassert (s == activatable);
      xassert (r == activatable + 1);
      xassert (v == activatable);
      xassert (found_app);

      reset_fds (fds, NUM_POLLEES);
      reset_fds_msg (fds, NUM_POLLFDS);
      r = g_poll (fds, NUM_POLLFDS, 0);
      check_fds (sockets, fds, NUM_POLLEES);
      xassert (r == 0);

      diff = times[i][1] - times[i][0];
      if (times_min > diff)
        times_min = diff;
      if (times_max < diff)
        times_max = diff;
      times_avg += diff;
      bucketize (diff, buckets, bucket_limits, BUCKET_COUNT);
      activatable = (activatable + 1) % NUM_POLLEES;
    }

  times_avg /= NUM_POLLEES;
  g_print ("variable socket number + msg poll time: \n%4lldns - %4lldns, average %4lldns\n", times_min, times_max, times_avg);
  print_buckets (buckets, bucket_limits, BUCKET_COUNT);

  cleanup_sockets (sockets, opp_sockets, NUM_POLLEES);
}

int
main (int   argc,
      char *argv[])
{
  int result;
  xmain_context_t *ctx;

  g_test_init (&argc, &argv, NULL);
  init_networking ();
  ctx = xmain_context_new ();

  g_test_add_func ("/gpoll/gpoll", test_gpoll);

  result = g_test_run ();

  xmain_context_unref (ctx);

  return result;
}
