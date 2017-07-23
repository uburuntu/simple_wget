///
/// Simple files downloader using libcurl
///

#include <stdlib.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <curl/multi.h>

static const int max_curr_jobs = 15;

size_t write_data (void *ptr, size_t size, size_t n, FILE *stream)
{
  size_t written = fwrite (ptr, size, n, stream);
  return written;
}

std::string url_to_filename (const std::string &url)
{
  size_t found = url.find_last_of ("/\\");
  return url.substr (found + 1);
}

static void init_easy_curl (CURLM *multi_handle, const std::string &url)
{
  CURL *eh = curl_easy_init();

  curl_easy_setopt (eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt (eh, CURLOPT_PRIVATE, url.c_str ());
  curl_easy_setopt (eh, CURLOPT_URL, url.c_str ());
  curl_easy_setopt (eh, CURLOPT_VERBOSE, 0L);
  curl_easy_setopt (eh, CURLOPT_WRITEDATA, fopen (url_to_filename (url).c_str (), "w+"));
  curl_easy_setopt (eh, CURLOPT_WRITEFUNCTION, write_data);

  curl_multi_add_handle (multi_handle, eh);
}

int main (int argc, char *argv[])
{
  CURLM *multi_handle;
  CURLMsg *msg;
  long int timeout;
  unsigned int current_job = 0;
  int max_fd, msgs_in_queue, running_jobs = -1;
  fd_set read_fd, write_fd, exec_fd;

  // Handle input data
  if (argc == 1)
    {
      printf ("Usage: %s <list of URLs>\n", argv[0]);
      return EXIT_SUCCESS;
    }
  std::vector<std::string> urls (argv + 1, argv + argc);

  // Init libcurl and its curls
  curl_global_init (CURL_GLOBAL_ALL);

  multi_handle = curl_multi_init ();

  curl_multi_setopt (multi_handle, CURLMOPT_MAXCONNECTS, (long) max_curr_jobs);

  for (current_job = 0; current_job < max_curr_jobs && current_job < urls.size (); current_job++)
    {
      init_easy_curl (multi_handle, urls[current_job]);
    }

  // Run main loop
  while (running_jobs)
    {
      curl_multi_perform (multi_handle, &running_jobs);

      if (running_jobs)
        {
          FD_ZERO (&read_fd);
          FD_ZERO (&write_fd);
          FD_ZERO (&exec_fd);

          if (curl_multi_fdset (multi_handle, &read_fd, &write_fd, &exec_fd, &max_fd))
            {
              printf ("\x1b[31mError\x1b[0m: curl_multi_fdset\n");
              return EXIT_FAILURE;
            }

          if (curl_multi_timeout (multi_handle, &timeout))
            {
              printf ("\x1b[31mError\x1b[0m: curl_multi_timeout\n");
              return EXIT_FAILURE;
            }

          if (timeout == -1)
            {
              timeout = 100;
            }

          if (max_fd == -1)
            {
              sleep ((unsigned int) timeout / 1000);
            }
          else
            {
              struct timeval fd_timeout = {timeout / 1000, (timeout % 1000) * 1000};

              if (select (max_fd + 1, &read_fd, &write_fd, &exec_fd, &fd_timeout) < 0)
                {
                  printf ("\x1b[31mError\x1b[0m: select error (%d)\n", max_fd + 1);
                  return EXIT_FAILURE;
                }
            }
        }

      while ((msg = curl_multi_info_read (multi_handle, &msgs_in_queue)))
        {
          if (msg->msg == CURLMSG_DONE)
            {
              char *url;
              double size, speed;

              CURL *e = msg->easy_handle;
              curl_easy_getinfo (e, CURLINFO_PRIVATE, &url);
              curl_easy_getinfo (e, CURLINFO_SIZE_DOWNLOAD, &size);
              curl_easy_getinfo (e, CURLINFO_SPEED_DOWNLOAD, &speed);

              printf ("\x1b[32mSuccess\x1b[0m: %s for <%s>\n", curl_easy_strerror (msg->data.result), url);
              printf ("\tDownloaded size: %.0f bytes\n", size);
              printf ("\tAvg. speed: %.0f bytes/sec\n\n", speed);

              curl_multi_remove_handle (multi_handle, e);
              curl_easy_cleanup (e);
            }
          else
            {
              printf ("\x1b[31mError\x1b[0m: curl msg: %d\n", msg->msg);
            }

          if (current_job < urls.size ())
            {
              init_easy_curl (multi_handle, urls[current_job++]);
              running_jobs++;
            }
        }
    }

  curl_multi_cleanup (multi_handle);
  curl_global_cleanup();

  return EXIT_SUCCESS;
}
